#!/usr/bin/php-cgi
<?php
echo "Content-Type: text/plain\r\n\r\n";

echo "Method: " . $_SERVER['REQUEST_METHOD'] . "\n";
echo "Query: " . $_SERVER['QUERY_STRING'] . "\n";
echo "Script: " . $_SERVER['SCRIPT_FILENAME'] . "\n";

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    echo "Body:\n";
    echo file_get_contents("php://stdin") . "\n";
}
?>