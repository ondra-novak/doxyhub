
function start() {
	
	
	"use strict";
	
	var prevStatus = "";
	
	function StatusChanger() {
		var prevStatus = null;
		this.set = function(element, status) {
			if (prevStatus === status) return false;
			if (prevStatus) {
				element.classList.remove(prevStatus);
			} else {
				element.classList.add("status");
			}
			element.classList.add(status);
			return true;
		}
		
		this.commit = function(status) {
			prevStatus = status;
		}
		this.getStatus = function() {
			return prevStatus;
		}
		
	};
	
	var sch = new StatusChanger();
	  
	function setStatus(status) {
		
		if (!sch.set(document.body, status)) return;

		sch.commit(status);

		if (status == "queued" || status == "building") {
			setProgress("bprogress",null);
			setProgress("qprogress",null);
		}
		else if (status == "done") {
			setProgress("qprogress",100);
		}
	}
	window.setStatus = setStatus;
	
	function setProgress(id, value, fake) {		
		if (timer) {
			clearTimeout(timer);
			timer = null;
		}
		var p = document.getElementById(id);
		if (value == null) {
			p.style.width = "auto";
			p.classList.add("unknown");
		}else {
			if (fake && value >=93) {
				value = (1-Math.pow(0.75,value*0.1))*100;
			}
			p.style.width = value+"%";
			p.classList.remove("unknown");
		}
	}


	
	var timer;
	
	function setProgressAutoTime(id, start_time, duration) {
		
		var now = Date.now();
		var ellapsed = now - start_time;
		var part = ellapsed*100.0/duration;
		setProgress(id, part,true);
		timer = setTimeout(setProgressAutoTime.bind(null,id,start_time,duration),Math.random()*5000);
	}
	
	function setFakeProgress(id, beg, en, start_time) {
		var t = Date.now()-start_time*1000; 
		var part = (en - beg)*(t/(t+50000));
		setProgress(id,part+beg,false);
	}
	
	window.setProgress = setProgress;
	window.setProgressAutoTime = setProgressAutoTime;
	
	
	function parseHash() {
		var h = location.hash;
		if (h.startsWith("#")) {
			var parts = h.substr(1).split("&");
			return parts.reduce(function(cur,item){
				
				var eq = item.indexOf('=');
				var key = eq<0? item:item.substr(0,eq);
				var value = item.substr(eq+1);
				cur[decodeURIComponent(key)] = decodeURIComponent(value);
				return cur;
			},{});
		} else {
			return null;
		}
	}
	
	
	
	var cur_server_status;
	
	function statusFetch() {
		fetch("api/status").then(function(res) {return res.json();})
		.then(function(data){
			console.log(data);
			var update_interval = 60000;
			if (data["status"] == "unknown") {
				var tmp = parseHash();
				if (tmp) {
					data["url"] = tmp["url"];
					data["branch"] = tmp["branch"];
				}
			}
			document.querySelector("#field_url").innerText = data["url"];
			document.querySelector("#field_branch").innerText = data["branch"];
			document.querySelector("#field_rev").innerText = data["rev"];
			var statfld = document.querySelector("#field_status");
			
			switch (data["status"]) {
			case "unknown":
				setStatus("unknown");
				statfld.innerText = "need rebuild";
				break;
			case "deleted":
				setStatus("deleted");
				statfld.innerText = "need rebuild";
				break;
			case "done":
				setStatus("done");
				statfld.innerHTML = "<a href='../'>available</a>";
				break;
			case "error":
				setStatus("error");
				statfld.innerText = "error";
				document.querySelector("#err_msg").innerText = document.getElementById("texts").getAttribute("data-err"+data.last_error);
				break;
			case "queued":
				setStatus("queued");
				statfld.innerText = "queued";
				update_interval = 2000;
				break;
			case "building":
				setStatus("building");
				statfld.innerText = "building";
				if (data.build_time.avg_duration) {
					var dur = data.build_time.avg_duration;
					setProgress("bprogress",(Date.now()-(data.build_time.start*1000))/(dur*10), true);
				} else {
					switch (data.stage) {
					case "checkrev": setFakeProgress("bprogress", 0,7,data.timestamp);break;
					case "download": setFakeProgress("bprogress", 7,30,data.timestamp);break;
					case "generate": setFakeProgress("bprogress", 30,90,data.timestamp);break;
					case "upload": setFakeProgress("bprogress", 90,100,data.timestamp);break;
					}					
				}
				document.querySelector("#stage").innerText = document.getElementById("texts").getAttribute("data-"+data.stage);
				update_interval = 2000;
				break;
			}
			
			statusFetch.timerId = setTimeout(statusFetch, update_interval)
			cur_server_status = data;
		}).catch(function(){
			statusFetch.timerId = setTimeout(statusFetch, 5000);
		});
		
		statusFetch.stop = function() {
			clearTimeout(statusFetch.timerId);
		}
		statusFetch.restart = function() {
			statusFetch.stop();
			statusFetch();
		}
	}
	
	statusFetch();
	
	function checkCaptcha() {
		return Promise.resolve("zebra");
	}
	
	function showError(x) {
		statusFetch.stop();
		setStatus("error");
			document.querySelector("#err_msg").innerText = x;
	}

	function build_action() {
		checkCaptcha().then(function(ccode) {
			var url;
			var data;
			if (cur_server_status.status == "unknown") {
				url = "api/create";
				data = {
						"url":cur_server_status.url,
						"branch":cur_server_status.branch,
						"captcha":ccode,				
				}
			} else {
				url = "api/build";
				data = {
						"captcha":ccode
						};				
			};
			
			fetch(url,{
				method: "POST",
				headers:{
		            "Content-Type": "application/json"
				},
				body: JSON.stringify(data),
			}).then(function(resp) {
				if (resp.status == 200 || resp.status==201) {
					resp.json().then(function(d){
						if (d.redir) {
							location.href = "../../"+d.id+"/";
						} else {
							statusFetch.restart();
						}
					});
				} else if (resp.status == 202 || resp.status == 409) {
					statusFetch.restart();					
				} else {
					showError(resp.status+" "+resp.statusText);
				}
			},function(err){
					showError(err.toString());				
			});
		});		
	}
	
	function showlog_action() {
		document.body.classList.add("showlogs");
	}
	function closelog_action() {
		document.body.classList.remove("showlogs");
	}
	
	document.querySelector("#build_button").addEventListener("click",build_action);
	document.querySelector("#show_log_button").addEventListener("click",showlog_action);
	document.querySelector("#closelogs").addEventListener("click",closelog_action);
	
	document.querySelectorAll("dh-showlogbox li").forEach(function(x){
		x.addEventListener("click",function(x){
			document.querySelectorAll("dh-showlogbox li").forEach(function(y){
				y.classList.remove("selected");
			});
			x.classList.add("selected");
		}.bind(null,x));
	},this)
}