<?php
/**
 * Created by PhpStorm.
 * User: wjuni
 * Date: 2017-08-31
 * Time: 오후 10:19
 */

require_once '../php/db_connect.php';
if(!key_exists('mode', $_POST) || !is_numeric($_POST['mode'])){
    http_response_code(405);
    exit;
}
$mode = intval($_POST['mode']);
$mysqli = get_mysqli();

if($mode === 1) {
    //start
    $stmt = $mysqli->prepare("UPDATE tbl_task SET `task_status`=0");
    echo $stmt->execute();
    $stmt->close();
    $stmt = $mysqli->prepare("INSERT INTO tbl_task (line_multi, line_yellow, recovery_mode, terminate_cond, terminate_param, task_start_time, task_status) VALUES (?, ?, ?, ?, ?, NOW(), 1);");
    $stmt->bind_param("iiiis", $_POST['multi'], $_POST['yellow'], $_POST['recovery'], $_POST['cond'], $_POST['param']);
    echo ($stmt->execute() ? $stmt->insert_id : '0');
} else if($mode === 2) {
    //stop
    //if (!key_exists('tid', $_POST) || !is_numeric($_POST['tid'])){
     //   http_response_code(405);
    //    exit;
    //}
    $stmt = $mysqli->prepare("UPDATE tbl_task SET `task_status`=0");
   // $stmt->bind_param("i", $_POST['tid']);
    echo $stmt->execute();
    $stmt->close();
}
$mysqli->close();
