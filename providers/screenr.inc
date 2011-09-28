<?php

/**
 * @file
 *   This include processes screenr.com media files for use by emfield.module.
 */

define('EMVIDEO_SCREENR_MAIN_URL', 'http://www.screenr.com/');

/**
 *  This defines the version of the content data array that we serialize
 *  in emvideo_screenr_data(). If we change the expected keys of that array,
 *  we must increment this value, which will allow older content to be updated
 *  to the new version automatically.
 */
define('EMVIDEO_SCREENR_DATA_VERSION', 1);

/**
 * hook emvideo_PROVIDER_info
 * this returns information relevant to a specific 3rd party video provider
 * @return
 *   an array of strings requested by various admin and other forms
 *   'name' => the translated name of the provider
 *   'url' => the url to the main page for the provider
 *   'settings_description' => a description of the provider that will be posted in the admin settings form
 *   'supported_features' => an array of rows describing the state of certain supported features by the provider.
 *      These will be rendered in a table, with the columns being 'Feature', 'Supported', 'Notes'.
 */
function emvideo_screenr_info() {
  $features = array(
    array(t('Autoplay'), t('Yes'), ''),
    array(t('RSS Attachment'), t('Yes'), ''),
    array(t('Thumbnails'), t('Yes'), t('')),
    array(t('Full screen mode'), t('Yes'), t('You may customize the player to enable or disable full screen playback. Full screen mode is enabled by default.')),
  );
  return array(
    'provider' => 'screenr',
    'name' => t('Screenr'),
    'url' => EMVIDEO_SCREENR_MAIN_URL,
    'settings_description' => t('These settings specifically affect videos displayed from !screenr.', array('!screenr' => l(t('Screenr.com'), EMVIDEO_SCREENR_MAIN_URL))),
    'supported_features' => $features,
  );
}

/**
 * hook emvideo_PROVIDER_settings
 * this should return a subform to be added to the emvideo_settings() admin settings page.
 * note that a form field will already be provided, at $form['PROVIDER'] (such as $form['screenr'])
 * so if you want specific provider settings within that field, you can add the elements to that form field.
 */
function emvideo_screenr_settings() {
}

/**
 * hook emfield_PROVIDER_data
 *
 * provides an array to be serialised and made available with $item elsewhere
 */
function emvideo_screenr_data($field, $item) {
  $xml = emfield_request_xml('screenr', 'http://www.screenr.com/api/oembed.xml?url=http%3A//www.screenr.com/'. $item['value'], array(), TRUE, FALSE, $item['value']);
dpm($xml);

  // We have to extract the embed code from the xml.
  $html = $xml['OEMBED']['HTML'][0];
dpm($html);
  preg_match('@screenr.com/([^"]+)@i', $xml['OEMBED']['HTML'][0], $matches);

  $data = array();

  // Create some 'field' version control.
  $data['emvideo_screenr_version'] = EMVIDEO_SCREENR_DATA_VERSION;

  // Be nice to make this an array for changing media:thumbnail?
  $data['thumbnail'] = $xml['OEMBED']['THUMBNAIL_URL'][0];
  $data['embed'] = $matches[1];

  // Fetch the metadata via a SOAP call. emfield_request_xml() is all
  // very well, but doesn't get all the data I want.
  $raw = media_screenr_request_metadata($data['embed']);
  $data['raw'] = $raw->SearchMediaResult[0];

  if ($data['raw'] !== FALSE) {
    $data['duration'] = intval($data['raw']->MediaDuration);
  }

  return $data;
}

/**
 *
 */
function emvideo_screenr_rss($item, $teaser = NULL) {
}

/**
 * hook emvideo_PROVIDER_extract
 * this is called to extract the video code from a pasted URL or embed code.
 * @param $embed
 *   an optional string with the pasted URL or embed code
 * @return
 *   either an array of regex expressions to be tested, or a string with the video code to be used
 *   if the hook tests the code itself, it should return either the string of the video code (if matched), or an empty array.
 *   otherwise, the calling function will handle testing the embed code against each regex string in the returned array.
 */
function emvideo_screenr_extract($embed = '') {
  // http://screenr.com/KqUs
  return array(
    '@\.screenr\.com/([^\?]+)@i',
  );
}

/**
 * hook emvideo_PROVIDER_duration.
 * returns the duration of the video.
 *
 * @param $item
 *   An array containing the video metadata.
 * @return
 *   The duration of the video.
 */
function emvideo_screenr_duration($item) {
  return isset($item['data']['duration']) ? $item['data']['duration'] : 0;
}

/**
 * hook emvideo_PROVIDER_embedded_link($video_code)
 * returns a link to view the video at the provider's site
 *  @param $video_code
 *    the string containing the video to watch
 *  @return
 *    a string containing the URL to view the video at the original provider's site
 */
function emvideo_screenr_embedded_link($video_code) {
  return 'http://www.screenr.com/'. $video_code;
}

/**
 * The embedded flash displaying the screenr video.
 */
function theme_emvideo_screenr_flash($item, $width, $height, $autoplay) {
  $output = '';
  if ($item['embed']) {
    $output = '<object width="'. $width .'" height="'. $height .'">';
    $output .= '<param name="movie" value="http://www.screenr.com/v/'. $item['data']['embed'] .'">';
    $output .= '<param name="wmode" value="transparent" />';
    $output .= '<param name="allowFullScreen" value="'. $fullscreen .'" />';
    $output .= '<param name="api" value="true" />';
    $output .= '<param name="allowscriptaccess" value="always" />';
    $output .= '<embed src="http://www.screenr.com/v/'. $item['data']['embed'] .'" type="application/x-shockwave-flash" allowFullScreen="'. $fullscreen .'"  allowscriptaccess="always" width="'. $width .'" height="'. $height .'" api="true"></embed>';
    $output .= '</object>';
  }
  return $output;
}

/**
 * hook emvideo_PROVIDER_thumbnail
 * returns the external url for a thumbnail of a specific video
 * TODO: make the args: ($embed, $field, $item), with $field/$item provided if we need it, but otherwise simplifying things
 *  @param $field
 *    the field of the requesting node
 *  @param $item
 *    the actual content of the field from the requesting node
 *  @return
 *    a URL pointing to the thumbnail
 */
function emvideo_screenr_thumbnail($field, $item, $formatter, $node, $width, $height) {
  return $item['data']['thumbnail'];
}

/**
 * hook emvideo_PROVIDER_video
 * this actually displays the full/normal-sized video we want, usually on the default page view
 *  @param $embed
 *    the video code for the video to embed
 *  @param $width
 *    the width to display the video
 *  @param $height
 *    the height to display the video
 *  @param $field
 *    the field info from the requesting node
 *  @param $item
 *    the actual content from the field
 *  @return
 *    the html of the embedded video
 */
function emvideo_screenr_video($embed, $width, $height, $field, $item, &$node, $autoplay) {
  $output = theme('emvideo_screenr_flash', $item, $width, $height, $autoplay);
  return $output;
}

/**
 * hook emvideo_PROVIDER_video
 * this actually displays the preview-sized video we want, commonly for the teaser
 *  @param $embed
 *    the video code for the video to embed
 *  @param $width
 *    the width to display the video
 *  @param $height
 *    the height to display the video
 *  @param $field
 *    the field info from the requesting node
 *  @param $item
 *    the actual content from the field
 *  @return
 *    the html of the embedded video
 */
function emvideo_screenr_preview($embed, $width, $height, $field, $item, $node, $autoplay) {
  $output = theme('emvideo_screenr_flash', $item, $width, $height, $autoplay);
  return $output;
}

/**
 * Implementation of hook_emfield_subtheme.
 */
function emvideo_screenr_emfield_subtheme() {
    $themes = array(
        'emvideo_screenr_flash'  => array(
            'arguments' => array('item' => NULL, 'width' => NULL, 'height' => NULL, 'autoplay' => NULL),
            'file' => 'providers/screenr.inc',
            'path' => drupal_get_path('module', 'media_screenr'),
        )
    );
    return $themes;
}