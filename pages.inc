<?php

require_once("utils.inc");
require_once("dbapi.inc");
require_once("requests.inc");




////////////////////////////////////////////////////////////////////////////////
//
// PAGES
//
////////////////////////////////////////////////////////////////////////////////

// Return a hash of ALL the data for a given page.
// Bad API: You need EITHER the pageid OR the url & label (optional).
function pageData($pageid, $url = null, $label = null, $urlhash = null) {
	global $gPagesTable;

	if ( $pageid ) {
		$query = "select * from $gPagesTable where pageid='$pageid';";
	}
	else if ( $url ) {
		if ( $label ) {
			$query = "select * from $gPagesTable where url='$url' and label='$label';";
		}
		else {
			// If there's no label we pull the LATEST results.
			$query = "select * from $gPagesTable where url='$url' and " . ( $urlhash ? "urlhash=$urlhash" : getUrlhashCond($url) ) . " order by pageid desc limit 1;";
		}
	}
	else {
		return null;
	}

	return doRowQuery($query);
}


function getPage($url, $crawlid, $pagesTable="") {
	global $gPagesTable, $gUrlsTable;

	if ( ! $pagesTable ) {
		$pagesTable = $gPagesTable;
	}
	$urlhash = doSimpleQuery("select urlhash from $gUrlsTable where urlOrig = '$url';");
	return doRowQuery("select * from $pagesTable where urlhash=$urlhash and url='$url' and crawlid=$crawlid limit 1;");
}


function pageFromWPT($wptid, $medianRun, $bRepeatView=false) {
	// Fetch the HAR file from WebPagetest.
	$wptServer = wptServer();
	$url = $wptServer . "export.php?test=$wptid&run=$medianRun&cached=" . ( $bRepeatView ? "1" : "0" ) . "&php=1";
	$response = fetchUrl($url);

	return pageFromHar($response);
}


// Return a page object.
// $sJson is the contents of a HAR file.
function pageFromHar($sJson) {
	if ( !$sJson ) {
		dprint("ERROR: JSON string empty.");
		return NULL;
	}

	$HAR = json_decode($sJson, true);
	if ( NULL == $HAR ) {
		dprint("ERROR: JSON decode failed.");
		return NULL;
	}

	$aPages = $HAR['log']['pages'];
	$pagecount = count($aPages);
	if ( 0 === $pagecount ) {
		dprint("ERROR: no pages in HAR.");
		return NULL;
	}

	$page = $aPages[0];

	// Copy some properties deep in the JSON structure to the top.
	$page['onContentLoaded'] = ( array_key_exists('_domContentLoadedEventStart', $page) && $page['_domContentLoadedEventStart'] ? $page['_domContentLoadedEventStart'] : NULL );
	$page['onLoad'] = $page['_docTime'];
	$aResources = $HAR['log']['entries'];
	$page['url'] = $aResources[0]['request']['url']; // ASSUME the first request is for the main URL
	$page['resources'] = resourcesFromHar($aResources, $page['id']);

	return $page;
}


?>
