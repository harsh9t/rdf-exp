//============================================================================
// Name        : RDFDataEncoder
// Version     :
// Copyright   : KAUST-Infocloud
// Description : Hello World in C++, Ansi-style
//============================================================================

#include "partitioner_store.h"
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

int getdir (string dir, vector<string> &files)
{
	DIR *dp;
	struct dirent *dirp;
	if((dp  = opendir(dir.c_str())) == NULL) {
		cout << "Error(" << errno << ") opening " << dir << endl;
		return errno;
	}

	while ((dirp = readdir(dp)) != NULL) {
		if(((int)dirp->d_type) == 8)
			files.push_back(string(dirp->d_name));
	}
	closedir(dp);
	return 0;
}

partitioner_store::partitioner_store() {
}

partitioner_store::~partitioner_store() {
	/*for (unsigned i = 0; i < rdf_data.size(); i++)
		delete rdf_data[i];*/
}

void partitioner_store::load_encode_rdf_data(string input_dir, string output_file_name) {
	print_to_screen(part_string);
	Profiler profiler;
	profiler.startTimer("load_rdf_data");
	boost::unordered_map<string, ll>::iterator map_it;
	ll so_id = 0;
	ll predicate_id = 0;
	ll num_rec = 0;
	triple tmp_triple;
	ofstream ofs;
	string input_file;
	Type::ID objectType;
	ll sid, pid, oid;
	vector<triple> tmp_data;
	string subject,predicate,object,objectSubType;
	vector<string> files = vector<string>();

	getdir(input_dir,files);
	ofs.open(output_file_name.c_str(), ofstream::out);
	for (unsigned int file_id = 0; file_id < files.size();file_id++) {
		input_file = string(input_dir+"/"+files[file_id]);
		ifstream fin(input_file.c_str());
		TurtleParser parser(fin);
		print_to_screen("Reading triples from file: " + input_file);
		try {
			while (true) {
				try {
					if (!parser.parse(subject,predicate,object,objectType,objectSubType))
						break;
				} catch (const TurtleParser::Exception& e) {
					cerr << e.message << endl;
					// recover...
					while (fin.get()!='\n') ;
					continue;
				}

				//lookup subject
				map_it = so_map.find(subject);
				if (map_it == so_map.end()) {
					so_map[subject] = so_id;
					sid = so_id;
					so_id++;
				}
				else{
					sid = map_it->second;
				}

				//lookup predicate
				map_it = predicate_map.find(predicate);
				if (map_it == predicate_map.end()) {
					predicate_map[predicate] = predicate_id;
					pid = predicate_id;
					predicate_id++;
				}
				else{
					pid = map_it->second;
				}

				//lookup object
				for(unsigned i = 0 ; i < object.length() ;i++){
					if(object[i] == '\n' || object[i] == '\r')
						object [i] = ' ';
				}
				map_it = so_map.find(object);
				if (map_it == so_map.end()) {
					if(objectType == 1){
						so_map["\""+object+"\""] = so_id;
					}
					else
						so_map[object] = so_id;
					oid = so_id;
					so_id++;
				}
				else{
					oid = map_it->second;
				}


				tmp_triple = triple(sid, pid, oid, objectType);
				tmp_data.push_back(tmp_triple);
				num_rec++;
				if (num_rec % 1000000 == 0) {
					this->dump_encoded_data(ofs, tmp_data);
					tmp_data.clear();
					cout<<"Finished "<<num_rec<<endl;
				}
			}
			this->dump_encoded_data(ofs, tmp_data);
			tmp_data.clear();
			fin.close();
		}catch (const TurtleParser::Exception&) {
			return ;
		}
	}
	ofs.close();


	total_data_size = num_rec;
	print_to_screen(part_string+"\nTotal number of triples: " + toString(this->total_data_size) + " records");
	profiler.pauseTimer("load_rdf_data");
	print_to_screen("Done with data encoding in " + toString(profiler.readPeriod("load_rdf_data")) + " sec");
	profiler.clearTimer("load_rdf_data");
	this->dump_dictionaries(output_file_name);
}

void partitioner_store::dump_dictionaries(string file_name) {
	print_to_screen(part_string);
	Profiler profiler;
	profiler.startTimer("dump_dictionaries");
	print_to_screen("Dumping Dictionaries!");
	dump_map(so_map, file_name+"verts_map.dic", true);
	dump_map(predicate_map, file_name+"predicate_map.dic", true);
	profiler.pauseTimer("dump_dictionaries");
	print_to_screen("Done with dumping dictionaries in " + toString(profiler.readPeriod("dump_dictionaries")) + " sec");
	profiler.clearTimer("dump_dictionaries");

}

void partitioner_store::dump_encoded_data(ofstream &output_stream, vector<triple> & data){
	print_to_screen(part_string);
	Profiler profiler;
	profiler.startTimer("dump_encoded_data");

	for(unsigned i = 0 ; i < data.size(); i++){
		output_stream<<data[i].print()<<endl;
	}

	profiler.pauseTimer("dump_encoded_data");
	print_to_screen("Done with dump_encoded_data in "+toString(profiler.readPeriod("dump_encoded_data"))+" sec");
	profiler.clearTimer("dump_encoded_data");
}
