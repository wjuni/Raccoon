<?php
/**
 * Created by PhpStorm.
 * User: wjuni
 * Date: 2017. 8. 10.
 * Time: PM 3:40
 */
require_once '../php/db_connect.php';
function fetch_current($bot_id){
    $mysqli = get_mysqli();
    $stmt = $mysqli->prepare("SELECT tbl_status.*, tbl_task.*, TIMESTAMPDIFF(SECOND, tbl_task.task_start_time, NOW()) AS acc_time from tbl_status LEFT JOIN tbl_task ON tbl_status.task_id = tbl_task.tid
WHERE TIMESTAMPDIFF(SECOND, tbl_status.record_time, NOW()) < 3 AND tbl_status.bot_id=? ORDER BY tbl_task.tid DESC LIMIT 1;");
    $stmt->bind_param("i", $bot_id);
    $stmt->execute();
    $res = $stmt->get_result();
    if($res->num_rows > 0) {
        $obj = $res->fetch_object();
        $stmt->close();
        $mysqli->close();
        return $obj;
    } else {
        $stmt->close();
        $mysqli->close();
        return null;
    }
}