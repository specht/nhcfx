function rgb_to_hex(rgb)
{
    return '#' + rgb.substr(4, rgb.indexOf(')') - 4).split(',').map((color) => String("0" + parseInt(color).toString(16)).slice(-2)).join('');
}

function api_call(url, data, callback, options)
{
    if (typeof(options) === 'undefined')
        options = {};
    
    if (typeof(window.please_wait_timeout) !== 'undefined')
        clearTimeout(window.please_wait_timeout);
    
    if (options.no_please_wait !== true)
    {
        // show 'please wait' message after 500 ms
        window.please_wait_timeout = setTimeout(function() {
            var div = $('<div>').css('text-align', 'center').css('padding', '15px').addClass('text-muted').html("<i class='fa fa-cog fa-spin'></i>&nbsp;&nbsp;Einen Moment bitte...");
            $('.api_messages').empty();
            $('.api_messages').append(div);
        }, 500);
    }
    
    var jqxhr = jQuery.post({
        url: url,
        data: JSON.stringify(data),
        contentType: 'application/json',
        dataType: 'json'
    });
    
    jqxhr.done(function(data) {
        clearTimeout(window.please_wait_timeout);
        $('.api_messages').empty();
        if (typeof(callback) !== 'undefined')
        {
            data.success = true;
            callback(data);
        }
    });
    
    jqxhr.fail(function(http) {
        clearTimeout(window.please_wait_timeout);
        $('.api_messages').empty();
        if (typeof(callback) !== 'undefined')
        {
            var error_message = 'unknown_error';
            try {
                error_message = JSON.parse(http.responseText)['error'];
            } catch(err) {
            }
            console.log(error_message);
            callback({success: false, error: error_message});
        }
    });
}

function render()
{
    var f = $('#f0').val();
    var options = {};
    options.f = [{f: f, color: rgb_to_hex($('#f0').next('.swatch').css('background-color'))}];
    options.dpi = parseInt($('#dpi').val());
    options.scale = parseFloat($('#scale').val());
    options.range = [];
    options.range.push(parseFloat($('#xmin').val()));
    options.range.push(parseFloat($('#ymin').val()));
    options.range.push(parseFloat($('#xmax').val()));
    options.range.push(parseFloat($('#ymax').val()));
    options.pen_points = parseInt($('#pen_points').val());
    options.line_width = parseFloat($('#line_width').val());
    options.dpi = parseFloat($('#dpi').val());
    options.aa_level = parseInt($('#aa_level').val());
    options.padding = [
        parseFloat($('#padding_top').val()),
        parseFloat($('#padding_right').val()),
        parseFloat($('#padding_bottom').val()),
        parseFloat($('#padding_left').val())];
    api_call('/api/render', options, function(data) {
        $('#graph').empty();
        $('#graph').html(data.svg);
        $('#dl_buttons').empty();
        var dl_svg = $('<a>').addClass('btn btn btn-secondary').attr('href', '/cache/' + data.tag + '.svg').html('Download SVG');
        $('#dl_buttons').append(dl_svg);
        window.location.hash = data.tag;
        window.current_tag = data.tag;
//             var searchParams = new URLSearchParams(window.location.search);
//             if (data.tag !== window.tag)
//             {
//                 searchParams.set("tag", data.tag);
//                 window.location.search = searchParams.toString();
//             }
    });
}

$(document).ready(function() {
    $('.sb_fx').show();
    $('.cn_fx').show();
    $('.mi').click(function(e) {
        $('.sb').hide();
        $('.cn').hide();
        $('.mi').removeClass('active');
        var which = $(e.target).attr('mi');
        $('.sb_' + which).show();
        $('.cn_' + which).show();
        $('.mi_' + which).addClass('active');
    });
    $('#render').click(function() {
        render();
    });
    $('#f0').keydown(function(e) {
        if (e.keyCode === 13)
            render();
    });
    jQuery.each(['sin(x)', 'cos(x)', 'tan(x)', 'x^2', '1/x', 
                 'ln(x)', 'e^x', 'sqrt(x)', 'sin(x) / x', 
                 '3x^2-2x^3', 
                 'y-x = cos(x+y)',
                 'r = 3', 
                 'r = 3 + sin(phi*2)^2',
                 'r = 3 + sin(phi * 4) * 0.5',
                 'r = 3 - pow(pow(sin(phi * 2), 2), 16)/2',
                 'r = 3 - sin(phi*3)^64',
                 'r = 2 + pow(sin(phi * 16 + r * 4), 8) * 2.5',
                 'r = 3 + sin(atan2(y, abs(x)) * 2) * 0.8',
                 'r = 3 + sin(5* phi)^11',
                 'r = 2.5 + ((phi+pi+0.2) % (pi/8)) * 2',
                 'r % 1 = 0.5 + cos(4phi) * 0.05 * r',
                 'phi = cos(r*3)*pi',
                 'phi = tan(3*r)/5',
                 'phi = cos(r*3)',
                 'atan2(y, abs(x)) = sin(r*3)^4',
                 'atan2(y, abs(x)) = sin(r*3)^11',
                 'atan2(y, abs(x)) = cos(r*3)*pi/2',
                 'atan2(y, abs(x)) = cos(r*3)*pi',
                 'atan2(y, abs(x)) = cos(r*30)',
                 'atan2(y, abs(x)) = tan(r*3)',
                 'atan2(-y, abs(x)) = -log(r*3)',
                 '(4/pi)*((1/1)*sin(2*pi*(pi/10)*x)+(1/3)*sin(6*pi*(pi/10)*x)+(1/5)*sin(10*pi*(pi/10)*x)+(1/7)*sin(14*pi*(pi/10)*x)+(1/9)*sin(18*pi*(pi/10)*x)+(1/11)*sin(22*pi*(pi/10)*x)+(1/13)*sin(26*pi*(pi/10)*x)+(1/15)*sin(30*pi*(pi/10)*x)+(1/17)*sin(34*pi*(pi/10)*x)+(1/19)*sin(38*pi*(pi/10)*x)+(1/21)*sin(42*pi*(pi/10)*x)+(1/23)*sin(46*pi*(pi/10)*x)+(1/25)*sin(50*pi*(pi/10)*x)+(1/27)*sin(54*pi*(pi/10)*x))'
                ], function(_, f) {
        var div = $('<a>').attr('id', 'gal_' + _).addClass('gallery_graph');
        $('.gallery_graphs').append(div);
        options = {};
        options.range = [-4, -4, 4, 4];
        options.padding = [5, 5, 5, 5];
        options.scale = 0.5;
        if (f.indexOf('=') === -1)
            f = 'y = ' + f;
        options.f = [{f: f, color: '#143b86'}, {f: f.replace('=', '<'), color: '#729fcf', opacity: 0.3}];
//         options.f = [{f: f, color: '#143b86'}];
        options.grid = [{space: 1.0, color: '#888', width: 0.05}];
        options.font_size = 2.5;
        options.tick_length = 0.7;
        api_call('/api/render', options, function(data) {
            $('#gal_' + _).html(data.svg);
            $('#gal_' + _).attr('href', '/#' + data.tag);
        });
    });
    $('.drawer-header').click(function(e) {
        var header = $(e.target);
        var drawer = header.next('.drawer');
        if (drawer.is(':visible'))
        {
            drawer.slideUp();
            header.addClass('closed');
        }
        else
        {
            drawer.slideDown();
            header.removeClass('closed');
        }
    });
    var tag = window.location.hash.substr(1);
    if (tag.length > 0)
    {
        api_call('/api/load', {tag: tag}, function(data) {
            console.log(data);
            $('#f0').val(data.info.f[0].f);
            $('#f0').next('.swatch').css('background-color', data.info.f[0].color);
            $('#render').click();
        });
    }
    else
    {
        $('#f0').val('sin(x)');
        $('#render').click();
    }
    window.onhashchange = function() {
        var tag = window.location.hash.substr(1);
        if (tag.length > 0 && window.current_tag != tag)
        {
            api_call('/api/load', {tag: tag}, function(data) {
                $('#f0').val(data.info.f[0].f);
                $('#render').click();
                $('.mi_fx').click();
            });
        }
        else
        {
            window.location.hash = window.current_tag;
        }
    };
//     var params = new URLSearchParams(window.location.search);
//     if (params.has('tag'))
//     {
//         api_call('/api/load', {tag: params.get('tag')}, function(data) {
//             console.log(data);
//             $('#f0').val(data.info.f[0].f);
//             $('#render').click();
//         });
//     }
//     else
//     {
//         $('#f0').val('atan2(y, abs(x)) = cos(r*3)');
//         $('#render').click();
//     }
    jQuery.each(cling_colors, function(_, color) {
        var div = $('<div>').addClass('color');
        div.append($('<div>').css('background-color', color[0])).append($('<span>').text(color[1]));
        $('.swatches').append(div);
        div.click(function(e) {
            var rgb = $(e.currentTarget).find('div').css('background-color');
            $('.color-input-sample').css('background-color', rgb);
            $('#color_input').val(rgb_to_hex(rgb));
            $('.color').removeClass('active');
            $(e.currentTarget).addClass('active');
        });
    });
    
    $('.mi-color-chooser').click(function(e) {
        $('#color_input').val('#143b86');
        $('#color_input').data('color_target', $(e.target).attr('color_target'));
        $('.color-input-sample').css('background-color', '#143b86');
        $('#color_modal').modal();
    });
    
    $('.btn-apply-color').click(function(e) {
        var target = $('#color_input').data('color_target');
        if (target.substr(0, 1) === 'f')
            $('#' + target).next('.swatch').css('background-color', $('#color_input').val());
        $('#color_modal').modal('hide');
    });
});

