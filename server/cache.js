/**
 * Created by rosenth on 09/07/16.
 */

Object.defineProperty(window, "Cookies", {
    get: function() {
        return document.cookie.split(';').reduce(function(cookies, cookie) {
            cookies[cookie.split("=")[0]] = unescape(cookie.split("=")[1]);
            return cookies
        }, {});
    }
});


function get_cachelist(sortorder) {
    $.ajax(apiurl + 'cache', {
        method: 'GET',
        headers: {Authorization: 'BEARER ' + token},
        dataType: 'json',
        contentType: 'application/json; charset=UTF-8',
        accepts: 'application/json',
        cache: false,
        data: {sort: sortorder},
        success: function (data, status, xhr) {
            var t = $('#list').empty();
            t.append($('<tr>')
                .append($('<th>'))
                .append($('<th>').append('Last access')
                    .append(' ')
                    .append($('<img>', {src: 'sort_up.png', id: 'atasc', title: 'ATASC'})
                        .on('click', function (ev) {
                            get_cachelist('atasc');
                        })
                    )
                    .append($('<img>', {src: 'sort_down.png', id: 'atdesc', title: 'ATDESC'})
                        .on('click', function (ev) {
                            get_cachelist('atdesc');
                        })
                    )
                )
                .append($('<th>').append('Canonical'))
                .append($('<th>').append('Original path'))
                .append($('<th>').append('Cache path'))
                .append($('<th>').append('Size')
                    .append(' ')
                    .append($('<img>', {src: 'sort_up.png', id: 'fsdesc'})
                        .on('click', function (ev) {
                            get_cachelist('fsdesc');
                        })
                    )
                    .append($('<img>', {src: 'sort_down.png', id: 'fsasc'})
                        .on('click', function (ev) {
                            get_cachelist('fsasc');
                        })
                    )
                )
                .append($('<th>'))
            );
            switch (sortorder) {
                case 'atasc':
                {
                    $('#atasc').css({display: 'none'});
                    break;
                }
                case 'atdesc':
                {
                    $('#atdesc').css({display: 'none'});
                    break;
                }
                case 'fsasc':
                {
                    $('#fsasc').css({display: 'none'});
                    break;
                }
                case 'fsdesc':
                {
                    $('#fsdesc').css({display: 'none'});
                    break;
                }
            }
            for (var i in data) {
                var tline = $('<tr>')
                    .append($('<td>').append(i))
                    .append($('<td>').append(data[i].last_access))
                    .append($('<td>').append(data[i].canonical))
                    .append($('<td>').append(data[i].origpath))
                    .append($('<td>').append(data[i].cachepath))
                    .append($('<td>').append(data[i].size))
                    .append($('<td>').append(
                        $('<input>', {type: 'checkbox'})
                            .addClass('delete')
                            .data('canonical', data[i].canonical)
                    ));
                t.append(tline);
            }
        },
        error: function(xhr, status, error) {
            alert(error);
        }
    });
}

$(function() {
    $('#delsel').on('click', function(event) {
        var delinfostr = '';
        var dels = [];
        $('.delete:checked').each(function(index, ele){
            delinfostr += $(ele).data('canonical') + "\n";
            dels.push($(ele).data('canonical'));
        });
        if (confirm(delinfostr)) {
            $.ajax(apiurl + 'cache', {
                method: 'DELETE',
                headers: {Authorization: 'BEARER ' + token},
                contentType: 'application/json; charset=UTF-8',
                accepts: 'application/json',
                cache: false,
                data: JSON.stringify(dels),
                dataType: 'json',
                success: function(data, status, xhr) {
                    if (data.status == 'OK') {
                        get_cachelist('atasc');
                    }
                    else {
                        alert(data.msg);
                    }
                },
                error: function(xhr, status, error) {
                    alert(error);
                }
            });
        }
    });

    $('#purge').on('click', function(event){
        if (confirm('Purge cache?')) {
            $.ajax(apiurl + 'cache', {
                method: 'DELETE',
                headers: {Authorization: 'BEARER ' + token},
                contentType: 'application/json; charset=UTF-8',
                accepts: 'application/json',
                cache: false,
                success: function(data, status, xhr) {
                    if (data.status == 'OK') {
                        $('#msgbox').css({display: 'block'}).empty().append('Deleted ' + data.n + ' files!')
                        get_cachelist('atasc');
                    }
                    else {
                        alert(data.msg);
                    }
                },
                error: function(xhr, status, error) {
                    alert(error);
                }
            });

        }
    });

    $('#stop').on('click', function(event) {
        if (confirm('Stop server?')) {
            $.ajax(apiurl + 'exit', {
                method: 'GET',
                headers: {Authorization: 'BEARER ' + token},
                contentType: 'application/json; charset=UTF-8',
                accepts: 'application/json',
                cache: false,
                success: function(data, status, xhr) {
                    if (data.status == 'OK') {
                        alert('SIPI stopped!')
                    }
                    else {
                        alert(data.msg);
                    }
                },
                error: function(xhr, status, error) {
                    alert(error);
                }
            });

        }

    });

    get_cachelist('atasc');
});

