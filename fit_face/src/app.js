Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('showConfiguration', function() {
  var url = 'http://stuguru.github.io/fitface/index2.html';
  console.log('Showing Config');
  Pebble.openURL(url);
});


Pebble.addEventListener('webviewclosed', function(e) {
  var configData = JSON.parse(decodeURIComponent(e.response));
  console.log('Configuration page returned: ' + JSON.stringify(configData));

  var backgroundColor = configData['background_color'];

  var dict = {};
  if(configData['high_contrast'] === true) {
    dict['KEY_HIGH_CONTRAST'] = configData['high_contrast'] ? 1 : 0;  // Send a boolean as an integer
  } else {
    dict['KEY_COLOR_RED_BG'] = parseInt(backgroundColor.substring(2, 4), 16);
    dict['KEY_COLOR_GREEN_BG'] = parseInt(backgroundColor.substring(4, 6), 16);
    dict['KEY_COLOR_BLUE_BG'] = parseInt(backgroundColor.substring(6), 16);
  }

  // Send to watchapp
  Pebble.sendAppMessage(dict, function() {
    console.log('Send successful: ' + JSON.stringify(dict));
  }, function() {
    console.log('Send failed!');
  });
});
