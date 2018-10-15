
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
	}
	window.setStatus = setStatus;
	
}