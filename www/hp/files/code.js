function start() {
	"use strict";
	
	
	var qurl = document.querySelector("#query_url");
	var qb = document.querySelector("#query_branch");
	var butt = document.querySelector("#run_query");
	
	butt.addEventListener("click", function(){

		var url = qurl.value;
		var branch = qb.value;
		butt.disabled = true;
		if ((url.startsWith("https://") || url.startWith("http://")) 
				&& branch.length && url.length>10) {
			
			var args = "url="+encodeURIComponent(url)+"&branch="+encodeURIComponent(branch);
			var sreq = "search?"+args;
			fetch(sreq)
				.then(function(x) {return x.json();})
				.then(function(result) {
					document.querySelector("dh-content").classList.add("unload");
					setTimeout(function() {
						var redir = result.id+"/";
						if (!result.found) {
							redir = redir+"console/index.html#"+args;
						}
						location.href = redir;
					},1500);
				})
				.catch(function(err) {
					alert("Error: "+err.toString());
					butt.disabled = false;
				});
		}
		
	});
	
	
}
