#!/usr/bin/php-cgi
<?php

header("Content-Type: text/html");

?>
<!DOCTYPE html>
<html>
<head><title>CGI Test</title></head>
<body>
    <h1>CGI PHP Test</h1>

    <h2>Server Info</h2>
    <p>Method: <?php echo $_SERVER['REQUEST_METHOD']; ?></p>
    <p>Script: <?php echo $_SERVER['SCRIPT_FILENAME']; ?></p>
    <p>Query: <?php echo $_SERVER['QUERY_STRING']; ?></p>
    <p>PHP Version: <?php echo phpversion(); ?></p>

    <?php if ($_SERVER['REQUEST_METHOD'] === 'POST'): ?>
    <h2>POST Body</h2>
    <pre><?php echo file_get_contents("php://stdin"); ?></pre>
    <?php endif; ?>

    <h2>All ENV</h2>
    <table border="1">
        <?php foreach ($_SERVER as $key => $val): ?>
        <tr><td><?php echo $key; ?></td><td><?php echo $val; ?></td></tr>
        <?php endforeach; ?>
    </table>
</body>
</html>