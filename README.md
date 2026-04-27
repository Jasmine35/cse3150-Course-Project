# cse3150-Course-Project

1. To start this project off, I wanted to organize the directory of what I think the final BGP simulator would look like
- I made a include and src directory to hold the header and main .cpp files
- Include and src hold the same same files, apart from the main.cpp file in the src folder. Everything else is split up into a graph subdirectory and bgp subdirectory

2. The next thing I wanted to do is to make sure cmake --build succeeds and have 1 fake test that passes so before writing any simulation code.
- CMakeLists.txt is there to define the project and set the C++ standard (using C++ 17)
- I then want to declare my main executable target that points to src/main.cpp (entry point of the whole simulation)
- CMake is where I also declare where the headers live (in include/) so any .cpp file can do # include "graph/AS.h" without any relative path hacks
- I used Google Test for this and FreshContent

3. Build the AS Graph
- I started with my AS.h file: I built a node struct that holds the ADN plus 3 neighbor sets (providers, customers, peers)
- Each AS has a ASN (implemented as uint32_t because the max bits are 32 and this ensures that), 3 sets of neighbors (providers_, customers_, and peers_ that stores the ASN numbers of the different nodes), and a propogation rank
- I then wrote the GraphParser to read the CAIDA file format and Graph to write everything together
- In GraphParser, I made a struct to represent a single ASRelationship, that has asn1, asn2, and the type_ (-1 if provider/customer, 0 if peer)
- GraphParser is also in charge of parsing the lines, whcih I did using stringstream and taking the 1st (asn1), 2md (asn2), and 3rd column (the column that describes the relationship type)
- Each relationship/line was stored in a vector of relationships
- I wrote Graph.h/Graph.cpp using SMART POINTERS (specifically unique_ptr) because the Graph owns the AS nodes and nothing else owns them
- I then added cycle-detection logic (to detect any provider/customer cycles)
- For cycle detection, I have 2 sets: visited (which tracks every node you've ever seen across all DFS traversals), and inStack(which tracks only the nodes in your current path)
- If you follow a CUSTOMER edge and land on a node that's already in inStack then it means you've found a path that loops back to the ancestor and it's a cycle
- I then wrote unit tests in test_graph.cpp to test the graph functionality before moving onto using real data
- I then wrote a small test with the CAIDA data to make sure the file could be read and parsed

4. Implement Announcements and Local RIB
- I first created Announcement.h with its 4 fields: prefic, AS-path, next-hop ASN, recieved-from relationship
- I then built an abstract Policy base class and the BGP cubclass
- The BGP object owns the local RIB(unordered_map<prefix, Announcement>) and the recieeved queue(unordered_map<prefix, vector<Announcements>>
- ASes with no customers would be zero - to implement this, I used a BFS and started from the bottom of the provider tree - I added this logic to the assignRanks method
- I also added a seeding method to Graph that would go into AS's local RIB before the propogations step
- For the propogation step: I split it up into 3 methods: propogateUp, PropogateAcrossPeers, and propogateDown
- I also added a dumpCSV method to the Graph file that puts the final output in a csv with columns asn, prefix, and as_path

5. Work on the Tiebreaker logic
- tiebreaker logic: customer > peer > provider -> shorter pth -> lower next-hop ASN when deciding whtether to replace an existing RIB entry
- write the CSV output(asn, prefix, as_path)
- For this implementation, I want to run the output against the bgpsimulator.com -> I used the example scenario and mmirrored the scenario in the main.cpp file so I can make sure the outputs for the simple example matched
- I implemented the function isBetter in BGP.cpp which will return based on (1) relationship priority (2) shorter AS path and (3) lower next-hop ASN wins
- I also verified this logic with bgpsimulator.com: from doing this I found an issue where toStore was created with the prepended ASN but then was stroing ann for both branches (an error that made the ASNs missing in their own path)

6. ROV extension (section 3 of the assignment)
- ROV inherits from BGP and overrides the announcement-processing step and instead drops any announcement where rov_invalid == true
- I made another class ROV that inherits from BGP
- the only overriding function I has was the receieveAnnouncment function that drops the rov_invalid announcements
- I also added a loadROVASNs function to the Graph that loads the ROV ASNs from file and assign the ROV policy to the ASes

7. Integration and cleanup
- After each pahse, I implemented unit tests within the test folder in my program to make sure I was on the right step
- After everything was implemented, I copied the datasets from bench (given) and the comparison file to truly test my output
- Note all tests passed (my guess would be because of my tiebreaker logic) and ghe path sotrd in the RIB is getting mutated
- If I had more time for this project, I would try to debug starting from there first

8. (HONORS CONVERSION) use WASm to put this simulator on a website with a real domain name so simulations can be purely run client-side - using Cloudfare pages

