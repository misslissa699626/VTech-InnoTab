//var remote_root="http://localhost:8927/"
var remote_root="/"

var streamInfoMap = {
	nominalBitrate: {label: "Nominal Bitrate", units: "bps"},
	averageBitrate: {label: "Average Bitrate", units: "bps"},
	instantBitrate: {label: "Instant Bitrate", units: "bps"},
	size:           {label: "Size",            units: "bytes"},
	duration:       {label: "Duration",        units: ""},
	sampleRate:     {label: "Sample Rate",     units: "Hz"},
	channelCount:   {label: "Channels",        units: ""},
	dataType:       {label: "Codec",           units: ""}
};

player = {
    seeking: false,
    volume: 100,
    setVolumePending: false,
    pendingVolume: 100,
    
    onStatusReceived: function(status) {
		$('#timecode').text(status.timecode);
		$('#player-state').text("["+status.state+"]");
		if (!player.seeking) {
			$('#seekbar').slider("value", status.position*100);
		}
		
		// stream info
		var html = '<table class="stream-info">';
		for (var propertyName in status.streamInfo) {
		    var attrName  = streamInfoMap[propertyName].label;
		    var attrUnits = streamInfoMap[propertyName].units;
		    var attrValue = status.streamInfo[propertyName];
		    if (attrUnits) attrValue += " "+attrUnits;
			html += '<tr><td class="stream-info-attr">'+attrName+'</td><td>'+attrValue+'</td></tr>';
		}
		html += '</table>';
		$('#stream-info').html(html);
		
		// refresh the status in 1 second
		setTimeout(player.getStatus, 1000);
	},

	onSeekSliderPushed: function() {
		player.seeking = true;
	},
	
    onSeekSliderChanged: function() {
    	position = $('#seekbar').slider("value");
        player.seek(parseInt(position));
        player.seeking = false;
    },
    
    onVolumeSliderChanged: function() {
    	volume = $('#volume').slider("value");
        player.setVolume(parseInt(volume));
    },

    onSetVolumeCompleted: function() {
    	player.setVolumePending = false;
        if (player.volume != player.pendingVolume) {
        	player.setVolume(player.volume);
        }
    },
    
	getStatus: function() {
	    $.getJSON(remote_root+"player/status?callback=?", player.onStatusReceived);
	},

	play: function() {
        $.get(remote_root+"player/play");
    },

    stop: function() {
        $.get(remote_root+"player/stop");
    },

    pause: function() {
        $.get(remote_root+"player/pause");
    },

    setInput: function(name) {
	    $.get(remote_root+"player/set-input?name="+escape(name)); 
    },
    
    seek: function(position) {
	    $.get(remote_root+"player/seek?position="+position); 
    },

    setVolume: function(volume) {
    	player.volume = volume;
    	if (player.setVolumePending) return;
    	player.setVolumePending = true;
    	player.pendingVolume = volume;
	    $.get(remote_root+"player/set-volume?volume="+volume, player.onSetVolumeCompleted); 
    }
};


$(function() {
	// change defaults for range, animate and orientation
	$.extend($.ui.slider.defaults, {
		range: "min",
		animate: true,
		orientation: "vertical"
	});

	// setup seekbar slider
	$("#seekbar").slider({
		value: 0,
		orientation: "horizontal",
		stop: player.onSeekSliderChanged,
		start: player.onSeekSliderPushed
	});

	// setup volume slider
	$("#volume").slider({
		value: 100,
		orientation: "horizontal",
		slide: player.onVolumeSliderChanged,
		stop: player.onVolumeSliderChanged
	});

	// setup graphic EQ
	$("#eq > span").each(function() {
		// read initial values from markup and remove that
		var value = parseInt($(this).text());
		$(this).empty();
		$(this).slider({
			value: value
		})
	});

	$('.player-button').hover(
		function(){ $(this).addClass('ui-state-hover'); }, 
		function(){ $(this).removeClass('ui-state-hover'); }
	);

	$('#play-button').click(
	    function(){
	      player.play();
		  player.getStatus();
	    }
	);
	
	$('#stop-button').click(
	    function(){
	      player.stop();
		  player.getStatus();
	    }
	);

	$('#pause-button').click(
	    function(){
	      player.pause();
		  player.getStatus();
	    }
	);

	$('#set-input-button').click(
	    function(){ 
	      var input_name = $('#input-name').val();
	      player.setInput(input_name);
	      player.play();
		  player.getStatus();
	    }
	);

	player.getStatus();
});