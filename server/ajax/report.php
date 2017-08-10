<?php
/**
 * Created by PhpStorm.
 * User: wjuni
 * Date: 2017. 8. 9.
 * Time: PM 3:35
 */
ini_set('display_errors', 1);
ini_set('display_startup_errors', 1);
error_reporting(E_ALL);
require_once '../php/db_connect.php';

if ($_SERVER['REQUEST_METHOD'] !== 'POST' || !key_exists('data', $_POST)) {
    http_response_code(405);
    exit;
}
$mysqli = get_mysqli();
$data = json_decode($_POST['data']);
$stmt = $mysqli->prepare("INSERT INTO tbl_status (bot_id, record_time, bot_status, damage_ratio, 
acc_distance, task_id, gps_lat, gps_lon, bot_battery, repair_module, bot_speed)
VALUES (?, NOW(), ?, ?, ?, ?, ?, ?, ?, ?, ?);");
$stmt->bind_param("iiiiiiiisi", $data->bid, $data->sta, $data->dam, $data->dis, $data->tid, $data->lat, $data->lon, $data->bat, $data->rep, $data->spd);
echo $stmt->execute();
$stmt->close();
$mysqli->close();
?>