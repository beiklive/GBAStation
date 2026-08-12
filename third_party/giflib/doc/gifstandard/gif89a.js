function ToggleVis(number) {
	// toggle visibility of a single chapter
	id = "p" + number;
	c = document.getElementById(id).className;
	c = (c == "" ? "hiddenparagraph" : "");
	document.getElementById(id).className = c;
}

function SetEveryVis(status) {
	// set visibility of every chapter
	c = (status == 0 ? "hiddenparagraph" : "");
	for(i = 0; i <= 35; i++) {
		document.getElementById("p" + i).className = c;
	}
}
