<?php
session_start(); // Start a session to test session handling

// Set a test cookie
setcookie("debug_cookie", "test_value", time() + 3600, "/");

// Capture raw POST data
// $rawPostData = file_get_contents("php://input");

// Generate a response
header("Content-Type: text/html");
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PHP CGI Debugger</title>
    <style>
        body { font-family: Arial, sans-serif; }
        pre { background:rgb(15, 8, 8); padding: 10px; border-radius: 5px; }
        h2 { border-bottom: 2px solid #333; padding-bottom: 5px; }
    </style>
</head>
<body>
    <h1>PHP CGI Debugger</h1>

    <h2>🔹 Server & CGI Variables</h2>
    <pre><?php print_r($_SERVER); ?></pre>

    <h2>🔹 GET Data</h2>
    <pre><?php print_r($_GET); ?></pre>

    <h2>🔹 POST Data</h2>
    <pre><?php print_r($_POST); ?></pre>

    <!-- <h2>🔹 Raw POST Data</h2>
    <pre><?php echo htmlspecialchars($rawPostData); ?></pre> -->

    <h2>🔹 Cookies</h2>
    <pre><?php print_r($_COOKIE); ?></pre>

    <h2>🔹 Session Data</h2>
    <pre><?php print_r($_SESSION); ?></pre>

    <h2>🔹 Environment Variables</h2>
    <pre><?php print_r($_ENV); ?></pre>

    <h2>🔹 PHP Configuration</h2>
    <pre><?php phpinfo(); ?></pre>

</body>
</html>
