<?php
/**
 * Created by PhpStorm.
 * User: wjuni
 * Date: 2017. 8. 9.
 * Time: PM 3:35
 */
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    exit;
}
var_dump($_POST);
?>