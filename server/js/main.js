var dbg_openmenu;

jQuery(function($){
    var map;
    var termination_mode = 'Nonstop';
    var started = false;
    var KAIST_N13 = new naver.maps.LatLng(36.373465, 127.359998);
    var current_location = null;
    var termination_marker = null;
    var range_circle = null;
    var current_location_marker = null;
    var centering = true;
    var onDataReceived = function() {};

    function prepareMaps(divId) {
        var mapOptions = {
            center: KAIST_N13,
            scrollWheel: false,
            mapDataControl: false,
            zoomControl: true,
            zoomControlOptions: {
                style: naver.maps.ZoomControlStyle.SMALL,
                position: naver.maps.Position.TOP_RIGHT
            },
            disableDoubleClickZoom: true,
            zoom: 12
        };

        map = new naver.maps.Map(divId, mapOptions);

        var customControl = new naver.maps.CustomControl('<a href="#" class="btn_mylct" onclick="return false;" id="btn_current_loc"><span class="spr_trff spr_ico_mylct">Current Location</span></a>', {
            position: naver.maps.Position.TOP_LEFT
        });

        customControl.setMap(map);



        naver.maps.Event.addListener(map, 'click', function(e) {
            if(termination_mode === 'Target') {
                if(termination_marker === null) {
                    termination_marker = new naver.maps.Marker({
                        position: e.coord,
                        map: map
                    });
                } else {
                    termination_marker.setPosition(e.coord);
                }
            }
        });
        naver.maps.Event.addListener(map, 'dragstart', function(e) {
            centering = false;
            $('#btn_current_loc').addClass('grayscale');
        });

        $('#btn_current_loc').on('click', function(e){
            if( current_location !== null)
                map.setCenter(current_location);
            centering = true;
            $('#btn_current_loc').removeClass('grayscale');
        });


    }
    var initial = true;

    function loadDashboard() {
        $('#item_dashboard').show();

        onDataReceived = function (parsed) {
            for (var property in parsed) {
                if (parsed.hasOwnProperty(property)) {
                    $('#dash_' + property).html(parsed[property]);
                }
            }
            if (initial){
                $(".counter").counterUp({
                    delay: 50,
                    time: 800
                });
                initial = false;
            }
        };

        var sparklineLogin = function () {
            $('#sparklinedash').sparkline([0, 5, 6, 10, 9, 12, 4, 9], {
                type: 'bar',
                height: '30',
                barWidth: '4',
                resize: true,
                barSpacing: '5',
                barColor: '#7ace4c'
            });
            $('#sparklinedash2').sparkline([0, 5, 6, 10, 9, 12, 4, 9], {
                type: 'bar',
                height: '30',
                barWidth: '4',
                resize: true,
                barSpacing: '5',
                barColor: '#7460ee'
            });
            $('#sparklinedash3').sparkline([0, 5, 6, 10, 9, 12, 4, 9], {
                type: 'bar',
                height: '30',
                barWidth: '4',
                resize: true,
                barSpacing: '5',
                barColor: '#11a0f8'
            });
        };
        var sparkResize;
        $(window).on("resize", function (e) {
            clearTimeout(sparkResize);
            sparkResize = setTimeout(sparklineLogin, 500);
        });
        sparklineLogin();
    }

    function loadControlPanel() {
        $('#item_control').show();

        $('#btn_start').click(function(e){
            var bt = $(this);
            var li = $('#loader_information');
            if (bt.html() === "Start") {
                li.show();
                li.html('Starting Task...');
                $('.preloader').show();
                setTimeout(function() {
                    $(".preloader").fadeOut();
                }, 1000);
                bt.html("Stop");
                bt.removeClass('btn-success');
                bt.addClass('btn-danger');
                started = true;
            } else {
                li.show();
                li.html('Stopping Task...');
                $('.preloader').show();
                setTimeout(function() {
                    $(".preloader").fadeOut();
                }, 1000);

                bt.html("Start");
                bt.removeClass('btn-danger');
                bt.addClass('btn-success');
                started = false;
            }
            $('.btn_options').prop("disabled",started);

        });

        $('#btn_line').click(function(e){
            var bt = $(this);
            if (bt.html() === "1 Line") {
                bt.html("2 Lines");
            } else {
                bt.html("1 Line");
            }
        });
        $('#btn_color').click(function(e){
            var bt = $(this);
            if (bt.html() === "Yellow") {
                bt.html("White");
                bt.removeClass('btn-warning');
            } else {
                bt.html("Yellow");
                bt.addClass('btn-warning');
            }
        });
        $('#btn_recovery').click(function(e){
            var bt = $(this);
            if (bt.html() === "Recovery") {
                bt.html("Inspection");
            } else {
                bt.html("Recovery");
            }
        });
        $('.btn_term_cond').click(function(e){
            $('.btn_term_cond').removeClass('btn-primary');
            var bt = $(this);
            bt.addClass('btn-primary');
            $('.opt').hide();
            $('.'+$(this).attr('id')).show();
            termination_mode = bt.html();
            if(termination_marker !== null) {
                if(bt.html() !== "Target")
                    termination_marker.setMap(null);
                else
                    termination_marker.setMap(map);
            }
            if(range_circle !== null){
                range_circle.setMap(null);
                range_circle = null;
            }
            if(bt.html() === "Distance"){
                showTerminateDistance();
            }
        });

        function showTerminateDistance() {
            var inp = $("#opt_dist_num");
            var value = parseInt(inp.val());
            var maxv = parseInt(inp.attr("max"));
            var minv = parseInt(inp.attr("min"));
            if(value > maxv) value = maxv;
            if(isNaN(value) || value < minv) value = minv;
            inp.val(value);
            if(range_circle !== null){
                range_circle.setMap(null);
                range_circle = null;
            }
            if(current_location !== null) {
                range_circle = new naver.maps.Circle({
                    map: map,
                    center: current_location,
                    radius: value,
                    strokeColor: 'yellowgreen',
                    strokeOpacity: 1,
                    strokeWeight: 3,
                    fillColor: 'yellowgreen',
                    fillOpacity: 0.3
                });
            }

        }

        $("#opt_dist_num").on("change", showTerminateDistance);

        onDataReceived = function (parsed) {
            $('#btn_status').html(parsed['status'] + ' (Ver '+ parsed['ver'] + ' / GPS:' + parsed['gps'] +')')
        };
    }

    var menu_open = false;

    $(document).ready(function () {
        "use strict";
        prepareMaps('naver-map');

        function hashChanged(e) {
            console.log(e);
            location.reload();
        }

        $(".left_menu").on('click', function() {
            location.reload();
        });

        if ("onhashchange" in window) { // event supported?
            window.onhashchange = function () {
                hashChanged(window.location.hash);
            }
        }
        else { // event not supported:
            var storedHash = window.location.hash;
            window.setInterval(function () {
                if (window.location.hash !== storedHash) {
                    storedHash = window.location.hash;
                    hashChanged(storedHash);
                }
            }, 100);

        }

        var mq = window.matchMedia("(max-width: 1170px)");
        mq.addListener(WidthChange);
        WidthChange(mq);
        function WidthChange(mq) {
            if (!mq.matches) {
                revertMenu();
            } else {
                closeMenu();
            }
        }

        function openMenu() {
            if (mq.matches) {
                var sidebar = $('.sidebar');
                sidebar.css('width', '240px');
                sidebar.css('transition', '.3s ease-in');
                $('.menu_background').removeClass('hidden');
                sidebar.css('left', '0');

                menu_open = true;
            }
        }

        function closeMenu() {
            if (mq.matches) {
                $('.menu_background').addClass('hidden');
                var sidebar = $('.sidebar');
                sidebar.css('width', '0');
                sidebar.css('left', '-240px');
                menu_open = false;
            }
        }

        function toggleMenu() {
            if(menu_open)
                closeMenu();
            else
                openMenu();
        }
        $('.menu_background').click(closeMenu);


        function revertMenu() {
            $('.menu_background').addClass('hidden');
            var sidebar = $('.sidebar');
            sidebar.css('left', '0');
            sidebar.css('width', '240px');
            menu_open = false;
        }

        $('.logo-a').click(toggleMenu);

        var hash = window.location.hash.substr(1);
        if(hash === "") {
            location.replace("#dashboard");
            location.reload();
        }
        if(hash === "dashboard") {
            loadDashboard();
        } else if (hash === "control") {
            loadControlPanel();
        }
        $.post("ajax/fetch_dashboard.php", function(data) {
            var parsed = JSON.parse(data);

        });
        setInterval(function(){
            $.post("ajax/fetch_dashboard.php", function(data) {
                var parsed = JSON.parse(data);
                onDataReceived(parsed);
                if (parsed['lat'] > 0 && parsed['lng'] > 0){
                    current_location = new naver.maps.LatLng(parsed['lat'], parsed['lng']);
                    if(centering)
                        map.setCenter(current_location);
                    if(current_location_marker === null) {
                        var markerOptions = {
                            position: current_location,
                            map: map,
                            icon: {
                                url: 'images/current_location.png',
                                size: new naver.maps.Size(80, 80),
                                origin: new naver.maps.Point(0, 0),
                                anchor: new naver.maps.Point(8, 8),
                                scaledSize: new naver.maps.Size(16, 16)
                            }
                        };
                        current_location_marker = new naver.maps.Marker(markerOptions);
                        centering = true;
                        $('#btn_current_loc').removeClass('grayscale');
                    } else {
                        current_location_marker.setPosition(current_location);
                    }

                } else {
                    if(current_location_marker !== null) {
                        current_location_marker.setMap(null);
                        current_location_marker = null;
                    }
                    current_location = null;
                }
            });
        }, 1000);
    });



});
