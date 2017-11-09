<?php
/**
 * Created by PhpStorm.
 * User: wjuni
 * Date: 2017. 7. 24.
 * Time: PM 5:30
 */

require_once 'fetch.php';
$result = array();
$result['damage'] = 0;
$result['speed'] = 0;
$result['dist'] = 0;
$result['batt'] = 0;
$result['status'] = "Offline";
$result['gps'] = "Off";
$result['time_h'] = 0;
$result['time_m'] = '00';
$result['repair_status'] = "Off";
$result['repair_info'] = "n/a";
$result['lat'] = 0; //36.373465;
$result['lng'] = 0; //127.359998;

echo json_encode($result);

?>