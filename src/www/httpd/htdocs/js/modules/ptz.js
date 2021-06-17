var APP = APP || {};

APP.ptz = (function($) {

    function init() {
        registerEventHandler();
        initPage();
        updatePage();
    }

    function registerEventHandler() {
        $(document).on("click", '#img-au', function(e) {
            move('#img-au', 'up');
        });
        $(document).on("click", '#img-al', function(e) {
            move('#img-al', 'left');
        });
        $(document).on("click", '#img-ar', function(e) {
            move('#img-ar', 'right');
        });
        $(document).on("click", '#img-ad', function(e) {
            move('#img-ad', 'down');
        });
        $(document).on("click", '#button-goto', function(e) {
            gotoPreset('#button-goto', '#select-goto');
        });
        $(document).on("click", '#button-save', function(e) {
            gotoPresetBoot('#button-save', '#PTZ_PRESET_BOOT');
        });
        $(document).on("click", '#button-set', function(e) {
            setPreset('#button-set', '#select-set');
        });
    }

    function move(button, dir) {
        $(button).attr("disabled", true);
        $.ajax({
            type: "GET",
            url: 'cgi-bin/ptz.sh?dir=' + dir,
            dataType: "json",
            error: function(response) {
                console.log('error', response);
                $(button).attr("disabled", false);
            },
            success: function(data) {
                $(button).attr("disabled", false);
            }
        });
    }

    function gotoPreset(button, select) {
        $(button).attr("disabled", true);
        $.ajax({
            type: "GET",
            url: 'cgi-bin/preset.sh?action=go_preset&num='+$(select + " option:selected").val(),
            dataType: "json",
            error: function(response) {
                console.log('error', response);
                $(button).attr("disabled", false);
            },
            success: function(data) {
                $(button).attr("disabled", false);
            }
        });
    }

    function gotoPresetBoot(button, select) {
        var saveStatusElem;
        let configs = {};

        saveStatusElem = $('#save-status');

        saveStatusElem.text("Saving...");

        configs["PTZ_PRESET_BOOT"] = $('select[data-key="PTZ_PRESET_BOOT"]').prop('value');

        var configData = JSON.stringify(configs);
        var escapedConfigData = configData.replace(/\\/g,  "\\")
                                          .replace(/\\"/g, '\\"');

        $.ajax({
            type: "POST",
            url: 'cgi-bin/set_configs.sh?conf=system',
            data: escapedConfigData,
            dataType: "json",
            success: function(response) {
                saveStatusElem.text("Saved");
            },
            error: function(response) {
                saveStatusElem.text("Error while saving");
                console.log('error', response);
            }
        });
    }

    function setPreset(button, select) {
        $(button).attr("disabled", true);
        $.ajax({
            type: "GET",
            url: 'cgi-bin/preset.sh?action=set_preset&num='+$(select + " option:selected").val()+'&name='+$('input[type="text"][data-key="PRESET_NAME"]').prop('value'),
            dataType: "json",
            error: function(response) {
                console.log('error', response);
                $(button).attr("disabled", false);
            },
            success: function(data) {
                $(button).attr("disabled", false);
                window.location.reload();
            }
        });
    }

    function initPage() {
        $.ajax({
            type: "GET",
            url: 'cgi-bin/get_configs.sh?conf=ptz_presets',
            dataType: "json",
            success: function(data) {
                html = "<select id=\"select-goto\">\n";
                for (let key in data) {
                    if (key != "NULL") {
                        splitted = data[key].split("|");
                        html += "<option value=\"" + key + "\">" + key + " - " + splitted[0] + "</option>\n";
                    }
                }
                html += "<option value=\"10\">10 - Center</option>\n";
                html += "<option value=\"11\">11 - Upper-Left</option>\n";
                html += "<option value=\"12\">12 - Upper-Right</option>\n";
                html += "<option value=\"13\">13 - Lower-Right</option>\n";
                html += "<option value=\"14\">14 - Lower-Left</option>\n";
                html += "</select>\n";
                document.getElementById("select-goto-container").innerHTML = html;

                html = "<select data-key=\"PTZ_PRESET_BOOT\" id=\"PTZ_PRESET_BOOT\">\n";
                html += "<option value=\"default\">Default position</option>\n";
                for (let key in data) {
                    if (key != "NULL") {
                        splitted = data[key].split("|");
                        html += "<option value=\"" + key + "\">" + key + " - " + splitted[0] + "</option>\n";
                    }
                }
                html += "<option value=\"10\">10 - Center</option>\n";
                html += "<option value=\"11\">11 - Upper-Left</option>\n";
                html += "<option value=\"12\">12 - Upper-Right</option>\n";
                html += "<option value=\"13\">13 - Lower-Right</option>\n";
                html += "<option value=\"14\">14 - Lower-Left</option>\n";
                html += "</select>\n";
                document.getElementById("select-goto-boot-container").innerHTML = html;

                html = "<select id=\"select-set\">\n";
                for (let key in data) {
                    if (key != "NULL") {
                        splitted = data[key].split("|");
                        html += "<option value=\"" + key + "\">" + key + " - " + splitted[0] + "</option>\n";
                    }
                }
                html += "</select>\n";
                document.getElementById("select-set-container").innerHTML = html;
            },
            error: function(response) {
                console.log('error', response);
            }
        });

        $.ajax({
            type: "GET",
            url: 'cgi-bin/get_configs.sh?conf=system',
            dataType: "json",
            success: function(response) {
                $.each(response, function (key, state) {
                    if(key=="PTZ_PRESET_BOOT")
                        $('select[data-key="' + key +'"]').prop('value', state);
                });
            },
            error: function(response) {
                console.log('error', response);
            }
        });

        interval = 1000;

        (function p() {
            jQuery.get('cgi-bin/snapshot.sh?base64=yes', function(data) {
                image = document.getElementById('imgSnap');
                image.src = 'data:image/jpeg;base64,' + data;
            })
            setTimeout(p, interval);
        })();
    }

    function updatePage() {
        $.ajax({
            type: "GET",
            url: 'cgi-bin/status.json',
            dataType: "json",
            success: function(data) {
                for (let key in data) {
                    if (key == "model") {
                        if ((data[key] == "GK-200MP2B") || (data[key] == "GK-200MP2C")) {
                            $('#ptz_description').show();
                            $('#ptz_available').hide();
                            $('#ptz_main').show();
                        } else {
                            $('#ptz_description').hide();
                            $('#ptz_available').show();
                            $('#ptz_main').hide();
                        }
                    }
                }
            },
            error: function(response) {
                console.log('error', response);
            }
        });
    }

    return {
        init: init
    };

})(jQuery);
