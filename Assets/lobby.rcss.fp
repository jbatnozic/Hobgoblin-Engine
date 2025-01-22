@spritesheet theme 
{
%%fp:spritesheet_begin

	%%fp:src: -f lobby-spritesheet.tga -w 1024 -h 1024

	%%fp:sprite: -n frame    -f ./frame-l.png
	%%fp:sprite: -n titlebar -f ./titlebar.png

%%fp:spritesheet_end
}

body
{
	font-family: LatoLatin;
	font-weight: bold;
	font-style: normal;
	font-size: 20dp;
	color: rgb(0, 0, 0);
}

body.window {
	decorator: image(frame);
}

div {
    display: block;
}

handle {
	display: block;
}

div#title-bar {
    position: absolute;
    top: -45dp;
}

div#title-bar span {
	display: inline-block;
	width: 100%;

	font-size: 22px;
	font-weight: bold;

	box-sizing: border-box;
    padding-left: 20dp;
	padding-right: 20dp;
	padding-top: 15dp;
	padding-bottom: 15dp;

	font-effect: shadow(2px 2px rgba(2, 2, 2, 0.425));
	color: white;

    decorator: image(titlebar);
}

div#window-interior-holder {
    display: inline-block;
    width: 97%;
	height: 100%;

    overflow-x: hidden;
    overflow-y: scroll;
}

.window-contents {
    padding-left: 20dp;
	padding-right: 20dp;
	padding-top: 20dp;
	padding-bottom: 20dp;
}

table {
	box-sizing: border-box;
	display: table;
}
tr {
	box-sizing: border-box;
	display: table-row;
}
tr:first-child {
	background-color: #a1a1a1;
}
td {
	box-sizing: border-box;
	display: table-cell;
}
col {
	box-sizing: border-box;
	display: table-column;
}
colgroup {
	display: table-column-group;
}
thead, tbody, tfoot {
	display: table-row-group;
}

.centred-column {
	text-align: center;
}
.button {
	text-align: center;
}
.button:hover {
	text-align: center;
	background-color: #70ceeb;
}
.button:active {
	text-align: center;
	background-color: #354449;
}

/* SCROLL BAR */

/* Fix the width and push the scrollbar back to the extents of the window. */
scrollbarvertical
{
	margin-top: 6px;
	margin-bottom: 6px;
	margin-right: -11px;
	width: 27px;
}

/* Decorate the slider track. */
scrollbarvertical slidertrack
{
	background-color: #a1a1a1;

    box-sizing: border-box;
    border-width: 2dp;
    border-color: #000000;
}

/* Darken the decorator on active. */
scrollbarvertical slidertrack:active
{
	background-color: #354449;

    box-sizing: border-box;
    border-width: 2dp;
    border-color: #000000;
}

/* Push the slider bar in 4 pixels from the left edge. Fix the width of the bar and make sure
   the height doesn't drop below 46 pixels; under that the decorator will start squishing the
   images. */
scrollbarvertical sliderbar
{
	margin-left: 0px;
	width: 27px;
	min-height: 46px;

	background-color: #354449;

    box-sizing: border-box;
    border-width: 2dp;
    border-color: #000000;
}

/* Animate the bar's decorator on hover. */
scrollbarvertical sliderbar:hover
{
	background-color: #748991;

    box-sizing: border-box;
    border-width: 2dp;
    border-color: #000000;
}

/* Animate the bar's decorator on active. */
scrollbarvertical sliderbar:active
{
	background-color: #3f2a9e;

    box-sizing: border-box;
    border-width: 2dp;
    border-color: #000000;
}

/* Fix the size of the 'page up' slider arrow and decorate it appropriately. */
scrollbarvertical sliderarrowdec
{
	width: 27px;
	height: 24px;

	background-color: #354449;

    box-sizing: border-box;
    border-width: 2dp;
    border-color: #000000;
}

/* Animate the arrows on hover. */
scrollbarvertical sliderarrowdec:hover
{
	background-color: #748991;

    box-sizing: border-box;
    border-width: 2dp;
    border-color: #000000;
}

/* Animate the arrows on active. */
scrollbarvertical sliderarrowdec:active
{
	background-color: #3f2a9e;

    box-sizing: border-box;
    border-width: 2dp;
    border-color: #000000;
}

/* Fix the size of the 'page down' slider arrow and decorate it appropriately. */
scrollbarvertical sliderarrowinc
{
	width: 27px;
	height: 24px;

	background-color: #354449;

    box-sizing: border-box;
    border-width: 2dp;
    border-color: #000000;
}

/* Animate the arrows on hover. */
scrollbarvertical sliderarrowinc:hover
{
	background-color: #748991;

    box-sizing: border-box;
    border-width: 2dp;
    border-color: #000000;
}

/* Animate the arrows on active. */
scrollbarvertical sliderarrowinc:active
{
	background-color: #3f2a9e;

    box-sizing: border-box;
    border-width: 2dp;
    border-color: #000000;
}
