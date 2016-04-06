<html><head><title>Boiler status page</title>
<link rel="stylesheet" type="text/css" href="style.css">
</head>
<body>
<div id="main">
<h1>BOILER status</h1>

<form method="post" action="BOILER.cgi">
BOILER:
<input type="submit" name="BOILER" value="ON">
<input type="submit" name="BOILER" value="OFF">
Currently: %BOILER_status%
</form>

<br><br><br><li><a href="index.tpl">Home page</a></li>
</div>
</body></html>
