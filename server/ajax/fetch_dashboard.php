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

$obj = fetch_current(1);
if($obj !== null) {
    if($obj->bot_status == 1) $result['status'] = "Running";
    else  $result['status'] = "Online";
    $result['damage'] = $obj->damage_ratio;
    $result['speed'] = $obj->bot_speed / 100.0;
    $result['dist'] = $obj->acc_distance;
    $result['batt'] = $obj->bot_battery;
    $result['gps'] = ($obj->gps_lat != 0 && $obj->gps_lat != 0) ? "On" : "Off";
    $result['time_h'] = $obj->acc_time/60;
    $result['time_m'] = ($obj->acc_time)%60;
    $result['repair_status'] = strlen($obj->repair_module) > 0 ? "On" : "Off";
    $result['repair_info'] = strlen($obj->repair_module) > 0 ? $obj->repair_module: "n/a";
    $result['lat'] = $obj->gps_lat; //36.373465;
    $result['lng'] = $obj->gps_lon; //127.359998;
}
echo json_encode($result);

?>