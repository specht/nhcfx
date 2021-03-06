#!/usr/bin/env ruby

# rm /raw/cache/*; time echo '{"f":[{"f":"atan2(y, x) = sin(r*3)*pi","color":"#204a87"}],"range":[-8,-4,8,4],"padding":[5,5,5,5],"scale":1.0,"tick_length":1.0,"font_size":3,"label_padding":0.1,"dpi":150,"line_width":0.4,"aa_level":2,"pen_points":8,"color":"#000000","grid":[{"space":0.5,"color":"#aaa","width":0.05},{"space":1.0,"color":"#888","width":0.05}]}' | ./nhcfx.rb
# rm /raw/cache/*; echo '{"f":[{"f": "r < 3", "color":"#204a87"}]}' | ./nhcfx.rb

require 'base64'
require 'digest'
require 'json'
require 'nokogiri'
require 'open3'
require 'tempfile'
require 'yaml'

class AffineTransform
    def initialize
        @m = [1.0, 0.0, 0.0, 
              0.0, 1.0, 0.0, 
              0.0, 0.0, 1.0]
    end
    
    def multiply(b, a)
        [a[0] * b[0] + a[1] * b[3] + a[2] * b[6],
         a[0] * b[1] + a[1] * b[4] + a[2] * b[7],
         a[0] * b[2] + a[1] * b[5] + a[2] * b[8],
         a[3] * b[0] + a[4] * b[3] + a[5] * b[6],
         a[3] * b[1] + a[4] * b[4] + a[5] * b[7],
         a[3] * b[2] + a[4] * b[5] + a[5] * b[8],
         a[6] * b[0] + a[7] * b[3] + a[8] * b[6],
         a[6] * b[1] + a[7] * b[4] + a[8] * b[7],
         a[6] * b[2] + a[7] * b[5] + a[8] * b[8]]
    end
    
    def scale(sx, sy)
        t = [sx, 0, 0,
             0, sy, 0,
             0, 0, 1]
        @m = multiply(@m, t)
    end
    
    def translate(dx, dy)
        t = [1, 0, dx,
             0, 1, dy,
             0, 0, 1]
        @m = multiply(@m, t)
    end
    
    def t(x, y)
        x = @m[0] * x + @m[1] * y + @m[2] * 1
        y = @m[3] * x + @m[4] * y + @m[5] * 1
        z = @m[6] * x + @m[7] * y + @m[8] * 1
        return (x / z), (y / z)
    end
end

def render_function_to_svg(options = {})
    options[:range] ||= [-8, -4, 8, 4]
    options[:padding] ||= [5, 5, 5, 5]
    options[:scale] ||= 1.0 # 1 unit equals n cm
    options[:tick_length] ||= 1.0
    options[:font_size] ||= 3
    options[:label_padding] ||= 0.1
    options[:dpi] ||= 150
    options[:line_width] ||= 0.4
    options[:aa_level] ||= 2
    options[:pen_points] ||= 8
    options[:color] ||= '#000000'
    options[:f] ||= []
    options[:grid] ||= [{'space' => 0.5, 'color' => '#aaa', 'width' => 0.05}, 
                        {'space' => 1.0, 'color' => '#888', 'width' => 0.05}]
    options[:x_labels_delta] ||= 1.0
    options[:y_labels_delta] ||= 1.0
    options[:render_background] = true unless options.include?(:render_background)
    options[:render_grid] = true unless options.include?(:render_grid)
    options[:render_x_axis] = true unless options.include?(:render_x_axis)
    options[:render_x_labels] = true unless options.include?(:render_x_labels)
    options[:render_y_axis] = true unless options.include?(:render_y_axis)
    options[:render_y_labels] = true unless options.include?(:render_y_labels)
    
    xmin, ymin, xmax, ymax = *options[:range]
    padding = options[:padding]
    scale = options[:scale]
    width = (xmax - xmin) * 10.0 * scale + padding[1] + padding[3]
    height = (ymax - ymin) * 10.0 * scale + padding[0] + padding[2]
    graph_width = (xmax - xmin) * 10.0 * scale # in mm
    graph_height = (ymax - ymin) * 10.0 * scale # in mm
    tick_length = options[:tick_length]
    font_size = options[:font_size]
    label_padding = options[:label_padding]
    dpi = options[:dpi]
    
    options_json = options.to_json
    tag = Digest::SHA1.hexdigest(options_json).to_i(16).to_s(36)[0, 10]
    dir = '/cache'
    FileUtils.mkdir(dir) unless File.directory?(dir)
    svg_path = File.join(dir, tag + '.svg')
    json_path = File.join(dir, tag + '.json')
    File.open(json_path, 'w') do |f|
        f.write(options_json)
    end
    
    options[:f].map! do |f|
        which = nil
        v = f['f']
        if v.include?('<') || v.include?('>')
            f['type'] = 1
        else
            f['type'] = 0
            unless v.include?('=')
                v += ' = y'
            end
            parts = v.split('=').map { |x| x.strip }
            v = "#{parts[0]} - (#{parts[1]})"
        end
        f['f'] = v
        f
    end
    
    STDERR.puts svg_path
    
    unless File.exists?(svg_path)

        t = AffineTransform.new
        t.scale(1, -1)
        t.translate(-xmin, ymax)
        t.scale(scale * 10, scale * 10)
        t.translate(padding[3], padding[0])
        
        def grid_lines(xml, t, dir, a0, a1, b0, b1, step, color, width)
            x = a0.floor
            while x <= a1.ceil do
                if dir == 0
                    sx0, sy0 = t.t(x, b0)
                    sx1, sy1 = t.t(x, b1)
                else
                    sx0, sy0 = t.t(b0, x)
                    sx1, sy1 = t.t(b1, x)
                end
                xml.line(:x1 => sx0, :y1 => sy0, :x2 => sx1, :y2 => sy1, 
                        :style => "stroke: #{color}; stroke-width: #{width}mm;")
                x += step
            end
        end
        
        pixel_width = (graph_width * dpi / 25.4).to_i
        pixel_height = (graph_height * dpi / 25.4).to_i
        dx = (xmax.to_f - xmin) / pixel_width
        dy = (ymin.to_f - ymax) / pixel_height
        
        options[:f].each do |function_entry|
            function = function_entry['f']
            function_name = function_entry['label']
            function_color = function_entry['color']
            function_color ||= options[:color]
            function_opacity = function_entry['opacity']
            function_opacity ||= function_entry['type'] == 1 ? 0.25 : 1.0
            args = [function_entry['type'], function, pixel_width, pixel_height, xmin, ymax, dx, dy,
                    options[:aa_level], options[:line_width] * dpi / 25.4,
                    options[:pen_points]].map { |x| x.to_s }
            
            STDERR.puts args.join(' ')
#             
            src_sha1 = Digest::SHA1.hexdigest(args.join(' '))
            function_entry[:src_sha1] = src_sha1
            fx_png_path = File.join(dir, src_sha1 + '.fx.png')
            fx_txt_path = File.join(dir, src_sha1 + '.fx.txt')
#             STDERR.puts fx_png_path
            unless File.exists?(fx_png_path) && File.exists?(fx_txt_path)
                File.open(fx_txt_path + '.arg', 'w') do |f|
                    f.puts args.map { |x| '"' + x + '"' }.join(' ')
                end
                Open3.pipeline(['/app_bin/nhcfx', *args, fx_txt_path], 
                            ["convert -size #{pixel_width}x#{pixel_height} -depth 8 gray:- \"#{fx_png_path}\""])
#                         t1 = Time.now.to_f; puts "Render raw: #{t1 - t0}"; t0 = t1
            end

            stdout, threads = Open3.pipeline_r(
                "convert \"#{fx_png_path}\" gray:/dev/stdout",
                "/app_bin/interleave #{function_color[1, 2].to_i(16)} #{function_color[3, 2].to_i(16)} #{function_color[5, 2].to_i(16)} #{(function_opacity * 255).to_i}",
                "convert -size #{pixel_width}x#{pixel_height} -depth 8 rgba:- png32:-")
            png_base64 = Base64.strict_encode64(stdout.read)
            stdout.close
            function_entry[:png] = png_base64
            raise 'error' unless threads.all? { |t| t.value.success? }
        end
#                         t1 = Time.now.to_f; puts "Interleave: #{t1 - t0}"; t0 = t1
        builder = Nokogiri::XML::Builder.new(:encoding => 'UTF-8') do |xml|
            o = {
                'xmlns:dc' => 'http://purl.org/dc/elements/1.1/',
                'xmlns:cc' => 'http://creativecommons.org/ns#',
                'xmlns:rdf' => 'http://www.w3.org/1999/02/22-rdf-syntax-ns#',
                'xmlns:svg' => 'http://www.w3.org/2000/svg',
                'xmlns:xlink' => 'http://www.w3.org/1999/xlink',
                'xmlns' => 'http://www.w3.org/2000/svg',
                'version' => '1.1',
                'viewBox' => "0 0 #{width} #{height}",
                'width' => "#{width}mm",
                'height' => "#{height}mm"
            }
            xml.svg(o) do
                xml.defs do
                    xml.clipPath(:id => 'a') do
                        xml.rect(:x => padding[3], :y => padding[0], 
                                :width => graph_width, :height => graph_height)
                    end
                end
                xml.g do
                    if options[:render_background]
                        xml.rect(:x => 0, :y => 0, :width => width, :height => height,
                                 :style => 'fill:#fff; stroke: none;')
                    end
                    
                    # draw 'lt' functions
                    options[:f].each do |function_entry|
                        function_name = function_entry['label']
                        function_color = function_entry['color']
                        function_color ||= options[:color]
                        function_opacity = function_entry['opacity']
                        function_opacity ||= 1.0
                        next unless function_entry['type'] == 1
                        xml.image('xlink:href' => 'data:image/png;base64,' + function_entry[:png], 
                                :x => padding[3], :y => padding[0],
                                :width => graph_width, :height => graph_height)
                    end
                    
                    if options[:render_grid]
                        # draw grid lines (clipped against graph area)
                        xml.g('clip-path': 'url(#a)') do
                            options[:grid].each do |grid|
                                grid_lines(xml, t, 0, xmin, xmax, ymin, ymax, grid['space'], grid['color'], grid['width'])
                                grid_lines(xml, t, 1, ymin, ymax, xmin, xmax, grid['space'], grid['color'], grid['width'])
                            end
                        end
                    end
                    
                    # draw X axis
                    if options[:render_x_axis]
                        grid_lines(xml, t, 1, 0, 0, xmin, xmax + 0.1 / scale, 1.0, '#000', 0.05)
                        # draw X arrow
                        x, y = t.t(xmax, 0)
                        xml.g(:transform => "translate(#{x + 4}, #{y})") do
                            xml.path(:d => "M 0,0 -3,-0.75 -3,0.75 z", :style => 'fill:#000; stroke: none;')
                        end
                        # draw X label
                        sx, sy = t.t(xmax, 0)
                        xml << "<text x='#{sx + 1.5}' y='#{sy + (label_padding + 1.0) * font_size + tick_length * 0.5}' text-anchor='middle' style='font-family: Arial; font-size: #{font_size}px;'>x</text>"
                    end
                    
                    # draw Y axis
                    if options[:render_y_axis]
                        grid_lines(xml, t, 0, 0, 0, ymin, ymax + 0.1 / scale, 1.0, '#000', 0.05)
                        # draw Y arrow
                        x, y = t.t(0, ymax)
                        xml.g(:transform => "translate(#{x}, #{y - 4})") do
                            xml.path(:d => "M 0,0 0.75,3 -0.75,3 z", :style => 'fill:#000; stroke: none;')
                        end
                        # draw Y label
                        sx, sy = t.t(0, ymax)
                        xml << "<text x='#{sx - tick_length * 0.5 - label_padding * font_size}' y='#{sy - (label_padding + 1.0) * font_size + tick_length}' text-anchor='end' style='font-family: Arial; font-size: #{font_size}px;'>y</text>"
                    end
                    
                    if options[:render_x_labels]
                        # draw X ticks and labels
                        x = xmin.floor + 1.0
                        while x <= xmax - 0.5 do
                            unless x == 0
                                sx, sy = t.t(x, 0)
                                xml.line(:x1 => sx, :y1 => sy, :x2 => sx, :y2 => sy + tick_length,
                                        :style => "stroke: #000; stroke-width: 0.075mm;")
                                lx = "#{x}"
                                while lx[-1] == '0' do lx.chop! end;
                                lx.chop! if lx[-1] == '.'
                                xml << "<text x='#{sx}' y='#{sy + (label_padding + 1.0) * font_size + tick_length}' text-anchor='middle' style='font-family: Arial; font-size: #{font_size}px;'>#{lx}</text>"
                            end
                            x += options[:x_labels_delta]
                        end
                    end
                    
                    if options[:render_y_labels]
                        # draw Y ticks and labels
                        y = ymin.floor + 1.0
                        while y <= ymax - 0.5 do
                            unless y == 0
                                sx, sy = t.t(0, y)
                                xml.line(:x1 => sx - tick_length, :y1 => sy, :x2 => sx, :y2 => sy,
                                        :style => "stroke: #000; stroke-width: 0.075mm;")
                                ly = "#{y}"
                                while ly[-1] == '0' do ly.chop! end;
                                ly.chop! if ly[-1] == '.'
                                xml << "<text x='#{sx - tick_length - label_padding * font_size}' y='#{sy + font_size * 0.4}' text-anchor='end' style='font-family: Arial; font-size: #{font_size}px;'>#{ly}</text>"
                            end
                            y += options[:y_labels_delta]
                        end
                    end

                    if options[:render_x_labels] || options[:render_y_labels]
                        # draw 0 label
                        sx, sy = t.t(0, 0)
                        xml << "<text x='#{sx - tick_length - label_padding * font_size}' y='#{sy + (label_padding + 1.0) * font_size + tick_length}' text-anchor='end' style='font-family: Arial; font-size: #{font_size}px;'>0</text>"
                    end
                    
                    # draw 'eq' functions
                    options[:f].each do |function_entry|
                        function_name = function_entry['label']
                        function_color = function_entry['color']
                        function_color ||= options[:color]
                        function_opacity = function_entry['opacity']
                        function_opacity ||= 1.0
                        next unless function_entry['type'] == 0
                        xml.image('xlink:href' => 'data:image/png;base64,' + function_entry[:png], 
                                :x => padding[3], :y => padding[0],
                                :width => graph_width, :height => graph_height)
                    end
                    
                    # draw function labels
                    options[:f].each do |function_entry|
                        function_name = function_entry['label']
                        function_color = function_entry['color']
                        function_color ||= options[:color]
                        function_opacity = function_entry['opacity']
                        function_opacity ||= 1.0
                        fx_txt_path = File.join(dir, function_entry[:src_sha1] + '.fx.txt')
                        label_pos = [-1, -1]
                        begin
                            label_pos = File.read(fx_txt_path).strip.split(' ').map { |x| x.to_i }
                        rescue
                        end
                        if label_pos != [-1, -1]
                            lx = label_pos[0]
                            ly = label_pos[1]
                            lsx, lsy = 0, 0
                            anchor = 'middle'
                            if ly == 0
                                lsy -= font_size * 0.2
                            elsif ly == pixel_height - 1
                                lsy += font_size * 1.0
                            end
                            if lx == 0
                                lsx -= font_size * 0.2
                                lsy += font_size * 0.4
                                anchor = 'end'
                            elsif lx == pixel_width - 1
                                lsx += font_size * 0.2
                                lsy += font_size * 0.4
                                anchor = 'start'
                            end
                            lx, ly = t.t(xmin + label_pos[0] * dx, label_y = ymax + label_pos[1] * dy)
                            xml << "<text x='#{lx + lsx}' y='#{ly + lsy}' text-anchor='#{anchor}' style='fill: #{function_color}; font-style: italic; font-family: Arial; font-size: #{font_size}px;'>#{function_name}</text>"
                        end
                    end
                end
            end
        end
        
        svg = builder.to_xml
        File.open(svg_path, 'w') do |f|
            f.write(svg)
        end
    end
    tag
end

if File.absolute_path(__FILE__) == File.absolute_path($0)
    args = Hash[JSON.parse(STDIN.read).map { |k, v| [k.to_sym, v] }]
    puts render_function_to_svg(args)
end
