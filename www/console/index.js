
function start() {
	
	"use strict";
	
	var prevStatus = "";
	
	function StatusChanger() {
		this.prevStatus = null;
		this.set = function(element, status) {
			if (this.prevStatus) {
				element.classList.remove(this.prevStatus);
			} else {
				element.classList.add("status");
			}
			element.classList.add(status);
		}
		
		this.commit = function(status) {
			this.prevStatus = status;
		}
	};
	
	var sch = new StatusChanger();
	  
	function setStatus(status) {
		
		sch.set(document.body, status)

		sch.commit(status);

		if (status == "queued" || status == "building") {
			setProgress(null);
		}
		else if (status == "done") {
			setProgress(100);
		}
	}
	window.setStatus = setStatus;
	
	function setProgress(value, fake) {		
		if (timer) {
			clearTimeout(timer);
			timer = null;
		}
		var allp = document.querySelectorAll("dh-progress > div");
		if (value == null) {
		allp.forEach(function(x){
			x.style.width = "";
			x.classList.add("unknown");
		});

		}else {
			if (fake && value >=93) {
				value = (1-Math.pow(0.75,value*0.1))*100;
			}
			allp.forEach(function(x){
				x.style.width = value+"%";
				x.classList.remove("unknown");
			})
		}
	}


	
	var timer;
	
	function setProgressAutoTime(start_time, duration) {
		
		var now = Date.now();
		var ellapsed = now - start_time;
		var part = ellapsed*100.0/duration;
		setProgress(part,true);
		timer = setTimeout(setProgressAutoTime.bind(null,start_time,duration),Math.random()*5000);
	}
	
	window.setProgress = setProgress;
	window.setProgressAutoTime = setProgressAutoTime;
	
}