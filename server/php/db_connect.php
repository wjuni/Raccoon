<?php
/**
 * Created by PhpStorm.
 * User: wjuni
 * Date: 2017. 8. 10.
 * Time: PM 2:31
 */
require_once __DIR__ . '/config.php';

function get_mysqli() {
    global $db_username, $db_password, $db_dbname;
    return new mysqli('localhost', $db_username, $db_password, $db_dbname);
}
