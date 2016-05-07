<html><head><title>Bathroom FAN status page</title>
<link rel="stylesheet" type="text/css" href="style.css">
</head>
<body>
<div id="main">
<h1>FAN status</h1>

<form method="post" action="FAN.cgi">
FAN:
<input type="submit" name="FAN" value="ON">
<input type="submit" name="FAN" value="OFF">
Currently: %FAN_status%
</form>

<br><br><br><li><a href="index.tpl">Home page</a></li>
</div>
</body></html>
