# crawler


The General Algorithm


Set<URL> seen; // Will be implemented as a bloom filter for better space
Queue<URL> frontier = getSeedList();

while(true) {
	URL url = frontier.pop_front();
	HTML content = download(url); // similar to SSL lab...
	ParsedHTML parsed = parse(html);
	
	save parsed.content to be indexed later
	
	for link in parsed.links which are not in seen:
		add link to frontier

	



} // end while
