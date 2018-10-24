var recaptcha_callback;
var recaptcha_error_callback;

function start() {
	
	
	"use strict";
	
	var prevStatus = "";
	var auto_open = false;
	
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
				var k = document.querySelector("#keep_console_open");
				if (auto_open && !k.checked)
					open_doc();
				break;
			case "error":
				setStatus("error");
				statfld.innerText = "error";
				document.querySelector("#err_msg").innerText = data.last_error+" "+document.getElementById("texts").getAttribute("data-err"+data.last_error);
				break;
			case "queued":
				auto_open = true;
				setStatus("queued");
				statfld.innerText = "queued";
				update_interval = 2000;
				break;
			case "building":
				auto_open = true;
				setStatus("building");
				statfld.innerText = "building";
				if (data.build_time.avg_duration) {
					var dur = data.build_time.avg_duration;
					setProgress("bprogress",(Date.now()-(data.build_time.start*1000))/(dur*10), true);
				} else {
					switch (data.stage) {
					case "checkrev": setFakeProgress("bprogress", 0,5,data.timestamp);break;
					case "download": setFakeProgress("bprogress", 5,15,data.timestamp);break;
					case "generate": setFakeProgress("bprogress", 15,90,data.timestamp);break;
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
		return new Promise(function(ok, cancel){
			
			recaptcha_callback = function(token) {
				ok(token);				
			}
			recaptcha_error_callback = function(e) {
				cancel(e);
			}
			
			grecaptcha.execute()
		});
	}
	
	function showError(x) {
		statusFetch.stop();
		setStatus("error");
			document.querySelector("#err_msg").innerText = x;
	}

	function build_action() {

		var b = document.querySelector("#build_button");
		b.disabled=true;
		setTimeout(function() {b.disabled=false;},3000);

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
				grecaptcha.reset();
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
				grecaptcha.reset();
				showError(err.toString());				
			});
		});		
	}
	
	function showlog_action() {
		document.body.classList.add("showlogs");
		document.querySelector("#showlogs").classList.add("shown");
		var ifrm = document.querySelector("#logviewer");
		ifrm.src = "";
		setTimeout(function() {
			ifrm.src = "api/log.html";
		},1);
	}
	function showmanifest_action() {
		document.body.classList.add("showlogs");
		document.querySelector("#showmanifest").classList.add("shown");
		document.querySelector("#manifest_progress").classList.add("shown");
		setTimeout(function() {
			fetch("..//manifest.txt").then(function(x) {
				return x.text();
			}).then(function(x) {
				var lines = x.split("\n").sort();
				var elem = document.querySelector("#manifest_content");
				elem.innerText = "";
				var frag = document.createDocumentFragment();
				lines.forEach(function(l){
					var z = document.createElement("div");
					var a = document.createElement("a");
					a.innerText = l;
					a.setAttribute("href","..//"+l);
					a.setAttribute("target" ,"_blank");
					z.appendChild(a);
					frag.appendChild(z);				
				});
				elem.appendChild(frag);
				document.querySelector("#manifest_progress").classList.remove("shown");
			});
		},900);
	}
	function closelog_action() {
		document.body.classList.remove("showlogs");
		document.querySelector("#showlogs").classList.remove("shown");
	}
	function closemanifest_action() {
		var elem = document.querySelector("#manifest_content");
		elem.innerText="";
		document.body.classList.remove("showlogs");
		document.querySelector("#showmanifest").classList.remove("shown");
	}

	function open_doc() {
		document.body.classList.add("showlogs");		
		setTimeout(function(){location.href="..";},1000);
	}
	
	
	document.querySelector("#build_button").addEventListener("click",build_action);
	document.querySelector("#show_log_button").addEventListener("click",showlog_action);
	document.querySelector("#closelogs").addEventListener("click",closelog_action);
	document.querySelector("#closemanifest").addEventListener("click",closemanifest_action);
	document.querySelector("#show_manifest").addEventListener("click",showmanifest_action);
	document.querySelector("#download_button").addEventListener("click",function(){location.href="api/log.txt";});
	document.querySelector("#open_doc").addEventListener("click",open_doc);
	

}