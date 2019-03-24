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
        var f = $('#f').val();
        var options = {};
        options.f = [{f: f, color: '#204a87'}];
        options.dpi = parseInt($('#dpi').val());
        options.scale = parseFloat($('#scale').val());
        console.log(options);
        api_call('/api/render', options, function(data) {
            $('#graph').empty();
            $('#graph').html(data.svg);
            $('#dl_buttons').empty();
            var dl_svg = $('<a>').addClass('btn btn btn-secondary').attr('href', '/cache/' + data.tag + '.svg').html('Download SVG');
            $('#dl_buttons').append(dl_svg);
        });
    });
    jQuery.each(['sin(x)', 'cos(x)', 'tan(x)', 'x^2', '1/x', 
                 'ln(x)', 'e^x', 'sqrt(x)', 'sin(x) / x', 
                 '3x^2-2x^3', 
                 'r = 3', 
                 'r = 3 + sin(phi*2)^2',
                 'r = 3 + sin(phi * 4) * 0.5',
                 'r = 3 - pow(pow(sin(phi * 2), 2), 16)/2',
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
                 'atan2(y, abs(x)) = log(r*3)',
                 '(4/pi)*((1 /1)*sin( 2*pi*(pi/10)*x)+(1 /3)*sin( 6*pi*(pi/10)*x)+(1 /5)*sin(10*pi*(pi/10)*x)+(1 /7)*sin(14*pi*(pi/10)*x)+(1 /9)*sin(18*pi*(pi/10)*x)+(1/11)*sin(22*pi*(pi/10)*x)+(1/13)*sin(26*pi*(pi/10)*x)+(1/15)*sin(30*pi*(pi/10)*x)+(1/17)*sin(34*pi*(pi/10)*x)+(1/19)*sin(38*pi*(pi/10)*x)+(1/21)*sin(42*pi*(pi/10)*x)+(1/23)*sin(46*pi*(pi/10)*x)+(1/25)*sin(50*pi*(pi/10)*x)+(1/27)*sin(54*pi*(pi/10)*x))'
                ], function(_, f) {
        var div = $('<div>').attr('id', 'gal_' + _).addClass('gallery_graph');
        $('.gallery_graphs').append(div);
        options = {};
        options.range = [-4, -4, 4, 4];
        options.padding = [5, 5, 5, 5];
        options.scale = 0.5;
        if (f.indexOf('=') === -1)
            f = 'y = ' + f;
        options.f = [{f: f.replace('=', '<'), color: '#729fcf', opacity: 0.3}, {f: f, color: '#204a87'}];
//         options.f = [{f: f, color: '#204a87'}];
        options.grid = [{space: 1.0, color: '#888', width: 0.05}];
        options.font_size = 2.5;
        options.tick_length = 0.7;
        api_call('/api/render', options, function(data) {
            $('#gal_' + _).html(data.svg);
        });
    });
    $('#f').val('atan2(y, abs(x)) = cos(r*3)');
//     $('#f').val('r < 3');
    $('#render').click();
});
