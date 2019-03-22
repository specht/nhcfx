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
    $('.mi').click(function() {
        $('#sb_fx').hide();
        $('#sb_doc').show();
        $('#cn_fx').hide();
        $('#cn_doc').show();
        $('.mi').removeClass('active');
        $('.mi_doc').addClass('active');
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
//     $('#f').val('atan2(y, abs(x)) = cos(r*3)');
    $('#f').val('r < 3');
    $('#render').click();
});
