<!doctype html>
<html class="no-js" lang="en">
    <head profile="http://www.w3.org/2005/10/profile">
        <meta charset="utf-8">
        <meta http-equiv="X-UA-Compatible" content="IE=edge">
        <title>Chordlet Editor</title>
        <meta name="description" content="">
        <meta name="viewport" content="width=device-width, initial-scale=1">

        <link rel="apple-touch-icon" href="apple-touch-icon.png">
		<link rel="icon" href="favicon.ico">

        <link rel="stylesheet" href="css/normalize.css">
        <link rel="stylesheet" href="css/main.css">
        <script src="js/vendor/modernizr-2.8.3.min.js"></script>
    </head>
    <body>
        <!--[if lt IE 8]>
            <p class="browserupgrade">You are using an <strong>outdated</strong> browser. Please <a href="http://browsehappy.com/">upgrade your browser</a> to improve your experience.</p>
        <![endif]-->

<div id="header">
	<ul>
		<li><a href="http://www.chordlet.com">Chordlet</a></li>
		<li><a href="http://www.chordlet.com/editor" class="active">Editor</a></li>
		<li><a href="http://www.chordlet.com/specification">Specification</a></li>
	</ul>
</div>

<div class="colmask threecol">
	<div class="colmid">
		<div class="colleft">
			<div class="col1">
				<div id="cl_container"></div>
			</div>
			<div class="col2"><!-- Left column --></div>
			<div class="col3">
				<!-- Right column -->
				<!--
				<div id="ads">
					<a href="http://matthewjamestaylor.com">
						<img src="mjt-125x125.gif" width="125" border="0" height="125" alt="Art and Design by Matthew James Taylor" />
					</a>
				</div>
				-->
			</div>
		</div>
	</div>
</div>

<div id="footer">
	<p><a href="http://www.chordlet.com">Chordlet</a> - Copyright &#169; 2015</p>
</div>

		<script src="//ajax.googleapis.com/ajax/libs/jquery/1.11.2/jquery.min.js"></script>
		<script>window.jQuery || document.write('<script src="js/vendor/jquery-1.11.2.min.js"><\/script>')</script>
		<link rel="stylesheet" href="https://ajax.googleapis.com/ajax/libs/jqueryui/1.11.2/themes/smoothness/jquery-ui.css" />
		<script src="https://ajax.googleapis.com/ajax/libs/jqueryui/1.11.2/jquery-ui.min.js"></script>
        <script src="js/plugins.js"></script>
        <script src="js/main.js"></script>
		<script src="js/vendor/webfontloader.js"></script>
        <script src="js/vendor/jspdf.min.js"></script>
        <script src="js/vendor/require.min.js"></script>
		<script src="js/vendor/ace/ace.js"></script>
        <script src="js/chordlet.js"></script>

        <script>
            (function(b,o,i,l,e,r){b.GoogleAnalyticsObject=l;b[l]||(b[l]=
            function(){(b[l].q=b[l].q||[]).push(arguments)});b[l].l=+new Date;
            e=o.createElement(i);r=o.getElementsByTagName(i)[0];
            e.src='//www.google-analytics.com/analytics.js';
            r.parentNode.insertBefore(e,r)}(window,document,'script','ga'));
            ga('create','UA-54784671-1','auto');ga('send','pageview');
        </script>
    </body>
</html>
