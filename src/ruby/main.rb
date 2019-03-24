require 'fileutils'
require 'json'
require 'open3'
require 'openssl'
require 'sinatra/base'
require 'socket'
require 'timeout'
require 'yaml'

require './nhcfx.rb'

class Main < Sinatra::Base
    def assert(condition, message = 'assertion failed')
        raise message unless condition
    end

    def test_request_parameter(data, key, options)
        type = ((options[:types] || {})[key]) || String
        assert(data[key.to_s].is_a?(type), "#{key.to_s} is a #{type}")
        if type == String
            assert(data[key.to_s].size <= (options[:max_value_lengths][key] || options[:max_string_length]), 'too_much_data')
        end
    end
    
    def parse_request_data(options = {})
        options[:max_body_length] ||= 256
        options[:max_string_length] ||= 256
        options[:required_keys] ||= []
        options[:optional_keys] ||= []
        options[:max_value_lengths] ||= {}
        data_str = request.body.read(options[:max_body_length]).to_s
        @latest_request_body = data_str.dup
        begin
            assert(data_str.is_a? String)
            assert(data_str.size < options[:max_body_length], 'too_much_data')
            data = JSON::parse(data_str)
            @latest_request_body_parsed = data.dup
            result = {}
            options[:required_keys].each do |key|
                assert(data.include?(key.to_s))
                test_request_parameter(data, key, options)
                result[key.to_sym] = data[key.to_s]
            end
            options[:optional_keys].each do |key|
                if data.include?(key.to_s)
                    test_request_parameter(data, key, options)
                    result[key.to_sym] = data[key.to_s]
                end
            end
            result
        rescue
            STDERR.puts "Request was:"
            STDERR.puts data_str
            raise
        end
    end
    
    def respond(hash = {})
        @respond_hash = hash
    end
    
    after '/api/*' do
        @respond_hash ||= {}
        response.body = @respond_hash.to_json
    end
    
    post '/api/render' do
        data = parse_request_data(
            :optional_keys => [
                :f, :range, :padding, :scale, :tick_length,
                :font_size, :label_padding, :dpi, :line_width,
                :aa_level, :pen_points, :color, :grid
            ],
            :max_body_length => 8192,
            :types => {
                :f => Array,
                :range => Array,
                :padding => Array,
                :scale => Numeric,
                :tick_length => Numeric,
                :font_size => Numeric,
                :label_padding => Numeric,
                :dpi => Numeric,
                :line_width => Numeric,
                :aa_level => Numeric,
                :pen_points => Numeric,
                :color => String,
                :grid => Array
            })
        tag = render_function_to_svg(data)
        respond(:tag => tag, :svg => File.read("/cache/#{tag}.svg"))
    end

    post '/api/load' do
        data = parse_request_data(:required_keys => [:tag])
        info = JSON.parse(File.read("/cache/#{data[:tag]}.json"))
        respond(:info => info)
    end

    # compile programs
    unless FileUtils.uptodate?('/app_bin/interleave', ['/src/interleave.c'])
        system("gcc -O1 -o /app_bin/interleave /src/interleave.c")
        system("strip --strip-all /app_bin/interleave")
    end
    unless FileUtils.uptodate?('/app_bin/nhcfx', ['/src/nhcfx.cpp'])
        STDERR.puts "Compiling nhcfx... (this may take a while)"
        system("gcc -O1 -o /app_bin/nhcfx -I/src/ext/exprtk/ /src/nhcfx.cpp -lstdc++ -lm")
        system("strip --strip-all /app_bin/nhcfx")
    end
    STDERR.puts "Ready."
    run! if app_file == $0
end
