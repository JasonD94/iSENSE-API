#include "include/API.h"

// Default constructor
iSENSE::iSENSE() {
  // Set these to default values for error checking.
  upload_URL = EMPTY;
  get_URL = EMPTY;
  get_UserURL = EMPTY;
  title = EMPTY;
  project_ID = EMPTY;
  dataset_ID = EMPTY;
  contributor_key = EMPTY;
  contributor_label = "label";
  email = EMPTY;
  password = EMPTY;
}


// Constructor with parameters
iSENSE::iSENSE(std::string proj_ID, std::string proj_title,
               std::string label, std::string contr_key) {
  set_project_ID(proj_ID);
  set_project_title(proj_title);
  set_project_label(label);
  set_contributor_key(contr_key);
}


// Similar to the constructor with parameters, but can be called at anytime
void iSENSE::set_project_all(std::string proj_ID, std::string proj_title,
                             std::string label, std::string contr_key) {
  set_project_ID(proj_ID);
  set_project_title(proj_title);
  set_project_label(label);
  set_contributor_key(contr_key);
}


// Set the Project ID, and the upload/get URLs as well.
void iSENSE::set_project_ID(std::string proj_ID) {
  project_ID = proj_ID;
  upload_URL = devURL + "/projects/" + project_ID + "/jsonDataUpload";
  get_URL = devURL + "/projects/" + project_ID;
  get_project_fields();
}


// The user should also set the project title
void iSENSE::set_project_title(std::string proj_title) {
  title = proj_title;
}


// This one is optional, by default the label will be "cURL".
void iSENSE::set_project_label(std::string proj_label) {
  contributor_label = proj_label;
}


// As well as the contributor key they will be using
void iSENSE::set_contributor_key(std::string proj_key) {
  contributor_key = proj_key;
}


/* Users should never have to call this method, as it is possible to
 * pull datasets and compare dataset names to get the dataset_ID.
 * Users should instead use the appendbyName methods.
 */
void iSENSE::set_dataset_ID(std::string proj_dataset_ID) {
  dataset_ID = proj_dataset_ID;
}


// Sets both email & password at once. Checks for valid email / password.
bool iSENSE::set_email_password(std::string proj_email,
                                std::string proj_password) {
  email = proj_email;
  password = proj_password;

  if (get_check_user()) {
    std::cout << "\nEmail and password are valid.\n";
    return true;
  }

  std::cerr << "\nError in: set_email_password()\n";
  std::cerr << "Your email and password are **not** valid.\n";
  std::cerr << "Try entering your password again.\n";
  std::cerr << "You also need to have created an account on iSENSE.\n";
  std::cerr << "See: http://rsense-dev.cs.uml.edu/users/new \n\n";

  return false;
}


// Extra function that the user can call to just generate an ISO 8601 timestamp
// It does not push back to the map of vectors. It merely returns a string,
// that users may grab and then send off to the push_back function.
std::string iSENSE::generate_timestamp(void) {
  time_t time_stamp;
  time(&time_stamp);
  char buffer[sizeof "2011-10-08T07:07:09Z"];

  // Generates the timestamp, stores it in buffer.
  // Timestamp is in the form of Year - Month - Day -- Hour - Minute - Seconds
  strftime(buffer, sizeof buffer, "%Y-%m-%dT%H:%M:%SZ", gmtime(&time_stamp));

  // Converts char array (buffer) to C++ string
  std::string cplusplus_timestamp(buffer);

  return cplusplus_timestamp;
}


// Resets the object and clears the map.
void iSENSE::clear_data(void) {
  upload_URL = EMPTY;
  get_URL = EMPTY;
  get_UserURL = EMPTY;
  title = EMPTY;
  project_ID = EMPTY;           // Set these to default values
  contributor_key = EMPTY;
  contributor_label = "label";
  email = EMPTY;
  password = EMPTY;

  std::cout << "\nClearing the data arrays.\n";

  map_data.clear();   // Clear the map_data

  /* Clear the picojson objects:
   * object: upload_data, fields_data;
   *  value: get_data, fields;
   *  array: fields_array;
   */

  // Under the hood picojson::objects are STL maps and
  // picojson::arrays are STL vectors.
  upload_data.clear();
  fields_data.clear();
  owner_info.clear();

  // Uses picojson's = operator to clear
  // the get_data object and the fields object.
  value new_object;
  get_data = new_object;
  fields = new_object;

  // Clear the field array (STL vector)
  fields_array.clear();
  media_objects.clear();
  data_sets.clear();
}


// Adds a string to the map, which keeps track of the data to be uploaded.
void iSENSE::push_back(std::string field_name, std::string data) {
  // Add the piece of data to the back of the vector with the given field name.
  map_data[field_name].push_back(data);
}


// Add a field name / vector of strings (data) to the map.
void iSENSE::push_vector(std::string field_name,
                         std::vector<std::string> data) {
  // This will store a copy of the vector<string> in the map.
  // If you decide to add more data, you will need to use the push_back method.
  map_data[field_name] = data;
}


// Searches for projects with the search term.
// Returns a vector with projects that show up.
std::vector<std::string> iSENSE::get_projects_search(std::string search_term) {

  std::string get_search = devURL + "/projects?utf8=true&search=" \
                        + search_term + "&sort=updated_at&order=DESC";
  // Vector of project titles.
  std::vector<std::string> project_titles;

  // This project will try using CURL to make a basic GET request to rSENSE
  CURL *curl = curl_easy_init();          // cURL object
  long http_code = 0;                     // HTTP status code
  MEMFILE* json_file = memfopen();        // Writing JSON to this file.
  char error[256];                        // Errors get written here

  if (curl) {
    // Set the GET URL, in this case the one be created above using the user's
    // email & password.
    curl_easy_setopt(curl, CURLOPT_URL, get_search.c_str());

    // Write errors to the "error" array
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, &error);

    // From the picojson example, "github-issues.cc".
    // Used  for writing the JSON to a file.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, memfwrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, json_file);

    // Perform the request
    curl_easy_perform(curl);

    // This will put the HTTP response code into the "http_code" variable.
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    // cout the http code.
    std::cout << "\nhttp code was: " << http_code << "\n\n";
  }

  /*  The iSENSE API gives us one response code to check against:
   *  Success: 200 OK
   *  If we do not get a code 200 from iSENSE, something went wrong.
   */

  // If we do not get a code 200, or cURL quits for some reason,
  // we didn't successfully get the project's fields.
  if (http_code != HTTP_AUTHORIZED) {
    std::cerr << "\nError in: get_projects_search(string search_term) \n";
    std::cerr << "Project search failed.\n";
    std::cerr << "Something with either curl, your internet connection, \n";
    std::cerr << "iSENSE or something else failed.\n";

    // Clean up cURL and close the memfile
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    memfclose(json_file);

    return project_titles;
  }

  // We got a code 200, so try and parse the JSON into a PICOJSON object.
  // Error checking for the parsing occurs below.
  std::string errors;
  value projects_json;

  // This will parse the JSON file.
  parse(projects_json, json_file->data, json_file->data + json_file->size, &errors);

  // If we have errors, print them out and quit.
  if (errors.empty() != true) {
    std::cerr << "\nError in: get_projects_search(string search_term)";
    std::cerr << "Error parsing JSON file.\n";
    std::cerr << "Error was: " << errors << "\n";

    // Clean up cURL and close the memfile
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    memfclose(json_file);

    return project_titles;
  }

  // Convert the JSON array (projects_json) into a vector of strings
  // (strings would be project titles);
  array projects_array = projects_json.get<array>();
  array::iterator it, the_begin, the_end;

  the_begin = projects_array.begin();
  the_end = projects_array.end();

  // Check and see if the projects_title JSON array is empty
  if (the_begin == the_end) {
    /*
     * Print an error and quit, we can't make a vector of
     * project titles to return if the JSON array ended up empty.
     * Probably wasn't any projects with that search term.
     */
    std::cerr << "\nError in: get_projects_search(string search_term) \n";
    std::cerr << "Project title array is empty.\n";

    // Clean up cURL and close the memfile
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    memfclose(json_file);

    return project_titles;  // this is an empty vector
  }

  for (it = projects_array.begin(); it != projects_array.end(); it++) {
    // Get the current object
    object obj = it->get<object>();

    // Grab the field name
    std::string name = obj["name"].get<std::string>();

    // Push the name back into the vector.
    project_titles.push_back(name);
  }

  // Clean up cURL and close the memfile
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  memfclose(json_file);

  // Return a vector of project titles
  return project_titles;
}


// Checks to see if the email / password is valid
bool iSENSE::get_check_user() {
  if (email == EMPTY || password == EMPTY) {
    std::cerr << "\nPlease set an email & password for this project.\n";
    return false;
  } else if (email.empty() || password.empty()) {
    std::cerr << "\nPlease set an email & password for this project.\n";
    return false;
  }

  // If we get here, an email and password have been set, so do a GET using
  // the email & password.
  get_UserURL = devURL + "/users/myInfo?email=" +
                email + "&password=" + password;

  // This project will try using CURL to make a basic GET request to rSENSE
  CURL *curl = curl_easy_init();          // cURL object
  long http_code = 0;                     // HTTP status code

  if (curl) {
    // Set the GET URL
    curl_easy_setopt(curl, CURLOPT_URL, get_UserURL.c_str());

    // Stop libcURL from outputting to stdio.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, suppress_output);

    curl_easy_perform(curl);    // Perform the request

    // This will put the HTTP response code into the "http_code" variable.
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  }

  /*  The iSENSE API gives us two response codes to check against:
   *  Success: 200 OK
   *  Failure: 401 Unauthorized
   */
  if (http_code == HTTP_AUTHORIZED) {
    // Clean up cURL
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    // Return success.
    return true;
  }

  // Clean up cURL
  curl_easy_cleanup(curl);
  curl_global_cleanup();

  // If curl fails, return false.
  return false;
}


// GET the project fields for a given project ID
bool iSENSE::get_project_fields() {
  if (project_ID == EMPTY || project_ID.empty()) {
    std::cerr << "Error - project ID not set!\n";
    return false;
  }

  get_URL = devURL + "/projects/" + project_ID;

  // This project will try using CURL to make a basic GET request to rSENSE
  // It will then save the JSON it recieves into a picojson object.
  CURL *curl = curl_easy_init();      // cURL object
  long http_code = 0;                 // HTTP status code
  MEMFILE* json_file = memfopen();    // Writing JSON to this file.
  char error[256];                    // Errors get written here

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, get_URL.c_str());
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, &error);

    // From the picojson example, "github-issues.cc".
    // Used  for writing the JSON to a file.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, memfwrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, json_file);

    // Perform the request, res will get the return code
    curl_easy_perform(curl);

    // This will put the HTTP response code into the "http_code" variable.
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  }

  /*  The iSENSE API gives us one response code to check against:
   *  Success: 200 OK
   *  Failure: 404 Not Found
   */

  if (http_code != HTTP_AUTHORIZED) {
    std::cerr << "Error in method: get_project_fields()\n";
    std::cerr << "GET project fields failed.\n";
    std::cerr << "Is the project ID you entered valid?\n";

    // Clean up cURL and close the memfile
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    memfclose(json_file);

    return false;
  }

  // We got a code 200, so try and parse the JSON into a PICOJSON object.
  // Error checking for the parsing occurs below.
  std::string errors;

  // This will parse the JSON file.
  parse(get_data, json_file->data,
        json_file->data + json_file->size, &errors);

  // If we have errors, print them out and quit.
  if (!errors.empty()) {
    std::cerr << "\nError parsing JSON file in method: get_project_fields()\n";
    std::cerr << "Error was: " << errors;
    return false;
  }

  // Save the fields to the field array
  fields = get_data.get("fields");
  fields_array = fields.get<array>();

  // Clean up cURL and close the memfile
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  memfclose(json_file);

  // Return true as we were able to successfully get the project's fields.
  return true;
}


// Given that a project ID has been set, this function
// makes a GET request and saves all the datasets & media objects
// into two picojson arrays.
// It will also update the fields for that given project ID
bool iSENSE::get_datasets_and_mediaobjects() {
  // Check that the project ID is set properly.
  // When the ID is set, the fields are also pulled down as well.
  if (project_ID == EMPTY || project_ID.empty()) {
    std::cerr << "\nError in method: get_datasets_and_mediaobjects()\n";
    std::cerr << "\nPlease set a project ID!\n";
    return false;
  }

  // The "?recur=true" will make iSENSE return:
  // ALL datasets in that project.
  // ALL media objects in that project
  // And owner information.
  get_URL = devURL + "/projects/" + project_ID + "?recur=true";

  // This project will try using CURL to make a basic GET request to rSENSE
  // It will then save the JSON it recieves into a picojson object.
  CURL *curl = curl_easy_init();      // cURL object
  long http_code = 0;                 // HTTP status code
  MEMFILE* json_file = memfopen();    // Writing JSON to this file.
  char error[256];                    // Errors get written here

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, get_URL.c_str());
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, &error);

    // From the picojson example, "github-issues.cc".
    // Used  for writing the JSON to a file.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, memfwrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, json_file);

    // Perform the request, res will get the return code
    curl_easy_perform(curl);

    // This will put the HTTP response code into the "http_code" variable.
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    // Clean up cURL
    curl_easy_cleanup(curl);
    curl_global_cleanup();
  }

  /*  The iSENSE API gives us one response code to check against:
   *  Success: 200 OK
   *  Failure: 404 Not Found
   */

  // If we do not get a code 200, or cURL quits for some reason,
  // we didn't successfully get the project's fields.
  if (http_code != HTTP_AUTHORIZED) {
    std::cerr << "\nError in: get_datasets_and_mediaobjects().\n";
    std::cerr << "GET project fields failed.\n";
    std::cerr << "Is the project ID you entered valid?\n";

    // Close the memfile
    memfclose(json_file);

    return false;
  }

  // We got a code 200, so try and parse the JSON into a PICOJSON object.
  // Error checking for the parsing occurs below.
  std::string errors;

  // This will parse the JSON file.
  parse(get_data, json_file->data, json_file->data + json_file->size, &errors);

  // close the memfile
  memfclose(json_file);

  // If we have errors, print them out and quit.
  if (errors.empty() != true) {
    std::cerr << "\nError in method: get_datasets_and_mediaobjects()\n";
    std::cerr << "Parsing JSON file failed. Error was: " << errors;
    return false;
  }

  // Save the fields to the field array
  fields = get_data.get("fields");
  fields_array = fields.get<array>();

  // Save the datasets to the datasets array
  value temp = get_data.get("dataSets");
  data_sets = temp.get<array>();

  // Save the media objects to the media objects array
  temp = get_data.get("mediaObjects");
  media_objects = temp.get<array>();

  // Save the owner info.
  temp = get_data.get("owner");
  owner_info = temp.get<object>();

  return true;
}


// Calls the get_datasets function, returns a vector of the data
// Must be given a valid iSENSE dataset name & field name
// Must have a project ID set.
std::vector<std::string> iSENSE::get_dataset(std::string dataset_name,
                                             std::string field_name) {
  std::vector<std::string> vector_data;

  // Make sure a valid project ID has been set
  if (project_ID == EMPTY || project_ID.empty()) {
    std::cerr << "\n\nError in method: get_dataset(string, string)\n";
    std::cerr << "Please set a project ID!\n";
    return vector_data;
  }

  // First call get_datasets_and_mediaobjects() and see if that is sucessful.
  if (!get_datasets_and_mediaobjects()) {
    // We failed. Let the user know, and return an empty map.
    std::cerr << "\n\nError in method: get_dataset(string, string)\n";
    std::cerr << "Failed to get datasets.\n";
    return vector_data;
  }

  // If we get here, we can try and find the field name,
  // and return a vector of data for that field name.
  array::iterator it, the_begin, the_end;

  the_begin = data_sets.begin();
  the_end = data_sets.end();

  // Check and see if the data_sets array is empty
  if (the_begin == the_end) {
    std::cerr << "\n\nError in method: get_dataset(string, string)\n";
    std::cerr << "Datasets array is empty.\n";
    return vector_data;  // this is an empty vector
  }

  std::string dataset_ID = get_dataset_ID(dataset_name);
  std::string field_ID = get_field_ID(field_name);

  // If either dataset ID or field ID threw an error, quit.
  if (dataset_ID == GET_ERROR || field_ID == GET_ERROR) {
    std::cerr << "\n\nUnable to return a vector of data.\n";
    std::cerr << "Either the dataset / field names are incorrect, \n";
    std::cerr << "Or the project ID is wrong.\n";

    return vector_data;
  }

  // If we make here, we can make a vector containing
  // data points and return that to the user.

  /* Note on why there's 3 for loops:
   * First for loop goes through all datasets in the project and
   * looks for one with the dataset name we found above.
   * We then have the object containing all the info about that dataset.
   * The second for loop then goes through the info object and
   * looks for the array of data points for this dataset.
   * The final for loop goes through this array and grabs
   * all the data points with the above field ID (field name) and
   * stores these data points into a vector, which is then returned to the user.
   */

  // This outer for loop is for going through all datasets in the project
  for (it = data_sets.begin(); it != data_sets.end(); it++) {
    // Get the current object
    object obj = it->get<object>();

    // Get that object's dataset ID to compare against
    std::string id = obj["id"].to_str();

    // If we have the right object, we can now start looking for the data array
    if (id == dataset_ID) {
      // obtain a const reference to the map (as seen on picoJSON github page)
      const object& cur_obj = it->get<object>();

      // This basically lets us iterate through the current object
      for (object::const_iterator i = cur_obj.begin();
           i != cur_obj.end(); i++) {
        // When we get here, we've found the data array! WOO HOO!
        if (i->first == "data") {
          // Now we need one final for loop to go through the array,
          // and push_back just the data points for our field name
          // (using the field ID we found above)
          array dataset_list = i->second.get<array>();

          for (array::iterator iter = dataset_list.begin();
              iter != dataset_list.end(); iter++) {
            // We make some tmp objects for getting datapoints, since the
            // dataset array for each dataset stores objects for each data point
            object tmp_obj = iter->get<object>();
            std::string data_pt = tmp_obj[field_ID].to_str();
            vector_data.push_back(data_pt);
          }

          // When we get here, we're finished. Finally return the vector of
          // data for the given field name.
          return vector_data;
        }
      }
    }
  }

  /* Note: should we not find the data we're looking for, this vector is empty.
   * Users should check the vector to make sure it is not empty &
   * look for any error messages that are printed.
   * Also note - if we get here, then we failed somewhere above
   * since we should have returned a vector of data points.
   */
  std::cerr << "\n\nError in method: get_dataset(string, string)\n";
  std::cerr << "Failed to get data points. \n";
  std::cerr << "Check the following & make sure they are correct:\n";
  std::cerr << "field name, dataset name, project ID\n";

  return vector_data;
}


std::string iSENSE::get_field_ID(std::string field_name) {
  // Grab all the fields using an iterator.
  // Similar to printing them all out below in the debug function.
  array::iterator it;

  // Check and see if the fields object is empty
  if (fields.is<picojson::null>() == true) {
    // Print an error and quit, we can't do anything if
    // the field array wasn't set up correctly.
    std::cerr << "\nError in method: get_field_ID()\n";
    std::cerr << "Field array wasn't set up.";
    std::cerr << "Have you pulled the fields off iSENSE?\n";
    return GET_ERROR;
  }

  // We made an iterator above, that will let us run through the fields
  for (it = fields_array.begin(); it != fields_array.end(); it++) {
    // Get the current object
    object obj = it->get<object>();

    // Grab the field ID and save it in a string/
    std::string field_ID = obj["id"].to_str();

    // Grab the field name
    std::string name = obj["name"].get<std::string>();

    // See if we can find the field name in this project
    if (name == field_name) {
      return field_ID;
    }
  }

  std::cerr << "\nError in method: get_field_ID()\n";
  std::cerr << "Unable to find the field ID for the given field name.\n";
  std::cerr << "Did you spell the field name right?\n";
  return GET_ERROR;
}


std::string iSENSE::get_dataset_ID(std::string dataset_name) {
  // Compare the dataset name the user provided with datasets in the project.
  // Use an iterator to go through all the datasets
  array::iterator it;

  // Check and see if the datasets object is empty
  // Need to find out how to do this using picojson arrays!

  // We made an iterator above, that will let us run through the fields
  for (it = data_sets.begin(); it != data_sets.end(); it++) {
    // Get the current object
    object obj = it->get<object>();

    // Grab the dataset ID and save it in a string
    std::string dataset_ID = obj["id"].to_str();


    // Grab the dataset name
    std::string name = obj["name"].get<std::string>();

    if (name == dataset_name) {
      // We found the name, so return the dataset ID
      return dataset_ID;
    }
  }

  std::cerr << "\nError in method: get_dataset_ID()\n";
  std::cerr << "Unable to find the dataset ID for the given dataset name.\n";
  std::cerr << "Did you spell the dataset name right?\n";
  return GET_ERROR;
}


// Call this function to POST data to rSENSE
bool iSENSE::post_json_key() {
  if(!empty_project_check(POST_KEY, "post_json_key")) {
    return false;
  }

  // Should make sure each vector is not empty as well, since I had issues
  // uploading if any ONE vector was empty. rSENSE complained about a nil class.

  upload_URL = devURL + "/projects/" + project_ID + "/jsonDataUpload";

  // Call the POST function, give it type 1
  // since this is a upload JSON by contributor key.
  int http_code = post_data_function(POST_KEY);

  /*
  *  The iSENSE API gives us two response codes to check against:
  *  Success: 200 OK (iSENSE handled the request fine)
  *  Failure: 401 Unauthorized (email/pw or contributor key was not valid)
  *  Failure: 422 Unprocessable Entity (email or contributor key was fine,
  *           but there was an issue with the request's formatting.
  *           Something in the formatting caused iSENSE to fail.)
  */

  if (http_code == HTTP_AUTHORIZED) {
    std::cout << "\n\nPOST request successfully sent off to iSENSE!\n";
    std::cout << "HTTP Response Code was: " << http_code << "\n";
    std::cout << "The URL to your project is: " << dev_baseURL;
    std::cout << "/projects/" << project_ID << "\n";
    return true;
  }

  // Quickly find the error message for the given http_code.
  check_http_code(http_code, "post_json_key()");
  return false;
}


/*  Append to a dataset using its dataset_ID.
 *  The dataset ID can be found on iSENSE by going to a project
 *  and clicking on a dataset.
 *  In the future, uploading JSON will return the dataset ID for this function
 */
bool iSENSE::append_key_byID(std::string dataset_ID) {
  if(!empty_project_check(APPEND_KEY, "append_key_byID")) {
    return false;
  }

  // Set the dataset_ID
  set_dataset_ID(dataset_ID);

  // Set the append API URL
  upload_URL = devURL + "/data_sets/append";

  // Call the POST function, give it type 2
  // since this is an append by contributor key.
  int http_code = post_data_function(APPEND_KEY);

   /*
    *  The iSENSE API gives us two response codes to check against:
    *  Success: 200 OK (iSENSE handled the request fine)
    *  Failure: 401 Unauthorized (email/pw or contributor key was not valid)
    *  Failure: 422 Unprocessable Entity (email or contributor key was fine,
    *           but there was an issue with the request's formatting.
    *           Something in the formatting caused iSENSE to fail.)
    */

  if (http_code == HTTP_AUTHORIZED) {
    std::cout << "\n\nPOST request successfully sent off to iSENSE!\n";
    std::cout << "HTTP Response Code was: " << http_code << "\n";
    std::cout << "The URL to your project is: " << dev_baseURL;
    std::cout << "/projects/" << project_ID << "\n";
    return true;
  }

  // Quickly find the error message for the given http_code.
  check_http_code(http_code, "append_key_byID()");
  return false;
}


/*
 *  Appends to a dataset using its dataset name, which can
 *  be used to find a dataset ID
 *  We can find the dataset ID by comparing against all the datasets
 *  in a given project until we find the dataset with the given name.
 *
 */
bool iSENSE::append_key_byName(std::string dataset_name) {
  if(!empty_project_check(APPEND_KEY, "append_key_byName")) {
    return false;
  }

  // We can now find the dataset ID by comparing
  // against all the datasets in this project.
  get_datasets_and_mediaobjects();    // First pull down the datasets

  // Call the get_dataset_ID function
  std::string dataset_ID = get_dataset_ID(dataset_name);

  // If we didn't get any errors, call the append by ID function.
  if (dataset_ID != GET_ERROR) {
    append_key_byID(dataset_ID);
    return true;
  }

  // If we got here, we failed to find that dataset name in the current project.
  std::cerr << "\nError in method: append_key_byName()\n";
  std::cerr << "Failed to find the dataset name in project # " << project_ID;
  std::cerr << "\nMake sure to type the exact name, as it appears on iSENSE.\n";
  return false;
}


// Post using a email / password
bool iSENSE::post_json_email() {
  if(!empty_project_check(POST_EMAIL, "post_json_email")) {
    return false;
  }

  // Make sure to set the upload URL!
  upload_URL = devURL + "/projects/" + project_ID + "/jsonDataUpload";

  // Call the POST function, give it type 3
  // since this is upload JSON by email & password.
  int http_code = post_data_function(POST_EMAIL);

  /*
    *  The iSENSE API gives us two response codes to check against:
    *  Success: 200 OK (iSENSE handled the request fine)
    *  Failure: 401 Unauthorized (email or contributor key was not valid)
    *  Failure: 422 Unprocessable Entity (email or contributor key was fine,
    *               but there was an issue with the upload for some reason.
    *               Something in the formatting caused iSENSE to fail.)
    */

  if (http_code == HTTP_AUTHORIZED) {
    std::cout << "\n\nPOST request successfully sent off to iSENSE!\n";
    std::cout << "HTTP Response Code was: " << http_code << "\n";
    std::cout << "The URL to your project is: " << dev_baseURL;
    std::cout << "/projects/" << project_ID << "\n";
    return true;
  }

  // Quickly find the error message for the given http_code.
  check_http_code(http_code, "post_json_email()");
  return false;
}


// Post append using email and password
bool iSENSE::append_email_byID(std::string dataset_ID) {
  if(!empty_project_check(APPEND_EMAIL, "append_email_byID")) {
    return false;
  }

  set_dataset_ID(dataset_ID);   // Set the dataset_ID

  // Change the upload URL to append rather than create a new dataset.
  upload_URL = devURL + "/data_sets/append";

  // Call the POST function with type 4 for appending via email.
  int http_code = post_data_function(APPEND_EMAIL);

  /*  The iSENSE API gives us two response codes to check against:
   *  Success: 200 OK (iSENSE handled the request fine)
   *  Failure: 401 Unauthorized (email/pw or contributor key not valid)
   *  Failure: 422 Unprocessable Entity (email or contributor key was fine,
   *               but there was an issue with the upload for some reason.)
   *               Something in the formatting caused iSENSE to fail.)
   */

  if (http_code == HTTP_AUTHORIZED) {
    std::cout << "\n\nPOST request successfully sent off to iSENSE!\n";
    std::cout << "HTTP Response Code was: " << http_code << "\n";
    std::cout << "The URL to your project is: " << dev_baseURL;
    std::cout << "/projects/" << project_ID << "\n";
    return true;
  }

  // Quickly find the error message for the given http_code.
  check_http_code(http_code, "append_email_byID()");
  return false;
}


/*  Appends to a dataset using its dataset name.
 *  We can find the dataset ID by comparing against all the datasets in
 *  a given project until we find the dataset with the given name.
 */
bool iSENSE::append_email_byName(std::string dataset_name) {
  if(!empty_project_check(APPEND_EMAIL,"append_email_byName")) {
    return false;
  }

  // We can now find the dataset ID by looking at all datasets in this project.
  get_datasets_and_mediaobjects();

  // Call the get_dataset_ID function
  std::string dataset_ID = get_dataset_ID(dataset_name);

  // If we didn't get any errors, call the append by ID function.
  if (dataset_ID != GET_ERROR) {
    append_key_byID(dataset_ID);
    return true;
  }

  // If we got here, we failed to find that dataset name in the current project.
  std::cerr << "\nError in method: append_email_byName\nFailed to find";
  std::cerr << "the dataset name in project # " << project_ID << "\n";
  std::cerr << "Make sure to type the exact name, as it appears on iSENSE. \n";
  return false;
}


// This function is called by the JSON upload function
// It formats the upload string
// Users should not have to call this function - API methods will,
// and will pass an int value indicating which API method they are using.
void iSENSE::format_upload_string(int post_type) {
  // Add the title + the correct formatting
  upload_data["title"] = value(title);

  // This is now a switch. Future API methods can be added here.
  switch (post_type) {
    case POST_KEY:
      upload_data["contribution_key"] = value(contributor_key);
      upload_data["contributor_name"] = value(contributor_label);
      break;

    case APPEND_KEY:
      upload_data["contribution_key"] = value(contributor_key);
      upload_data["id"] = value(dataset_ID);
      break;

    case POST_EMAIL:
      upload_data["email"] = value(email);
      upload_data["password"] = value(password);
      break;

    case APPEND_EMAIL:
      upload_data["email"] = value(email);
      upload_data["password"] = value(password);
      upload_data["id"] = value(dataset_ID);
      break;
  }

  // Add each field, with its field ID and an array of all the data.

  // Grab all the fields using an iterator.
  // Similar to printing them all out below in the debug function.
  array::iterator it;

  // Pointer to one of the vectors in the map
  std::vector<std::string> *vect;

  // Check and see if the fields object is empty
  if (fields.is<picojson::null>() == true) {
    std::cerr << "\nError in method: format_upload_string()\n";
    std::cerr << "Field array wasn't set up.\n";
    std::cerr << "Have you pulled the fields off iSENSE?\n";
    return;
  }

  // We made an iterator above, that will let us run through the fields
  for (it = fields_array.begin(); it != fields_array.end(); it++) {
    // Get the current object
    object obj = it->get<object>();

    // Grab the field ID and save it in a string/
    std::string field_ID = obj["id"].to_str();

    // Grab the field name
    std::string name = obj["name"].get<std::string>();

    // Now add all the data in that field's vector (inside the map)
    // to the fields_array object.
    vect = &map_data[name];
    format_data(vect, it, field_ID);
  }

  // Once we've made the field_data object, we can
  // add the field_data object to the upload_data object
  upload_data["data"] = value(fields_data);
}


// This makes the switch above shorter,
// since I reuse this code for all 5 types of data.
void iSENSE::format_data(std::vector<std::string> *vect,
                         array::iterator it, std::string field_ID) {

  std::vector<std::string>::iterator x;

  // picojson::value::array, basically a vector but represents a json array.
  value::array data;

  // First we push all the vector data into a json array.
  for (x = vect -> begin(); x < vect -> end(); x++) {
    data.push_back(value(*x));
  }

  // Now we push the json array to the upload_data object.
  fields_data[field_ID] = value(data);
}


/*  This function is called by all of the POST functions.
 *  It must be given a parameter, an integer "type", which determines which
 *  way the JSON should be formatted in the format_upload_string function.
 *
 *  The method returns an HTTP response code, like "200", "404", "503", etc.
 */
int iSENSE::post_data_function(int post_type) {
  // Upload_URL must have already been set.
  // Otherwise the POST request will fail unexpectedly.
  if (upload_URL == EMPTY || upload_URL.empty()) {
    std::cerr << "\nError in method: post_data_function()\n";
    std::cerr << "Please set a valid upload URL.\n";
    return CURL_ERROR;
  }

  // Format the data to be uploaded. Call another function to format this.
  format_upload_string(post_type);

  /*  Once we get the data formatted, we can try to POST to rSENSE
   *    The below code uses cURL. It
   *    1. Sets the headers, so iSENSE knows we are sending it JSON
   *    2. Does some curl init stuff that makes the magic happen.
   *    3. cURL sends off the request, we can grab the return code to see
   *       if cURL failed.
   *       Also check the curl verbose debug to see why something failed.
   *    4. We also get the HTTP status code so we know if iSENSE handled
   *       the request or not.
   */

  // CURL object and response code.
  CURL *curl = curl_easy_init();          // cURL object
  long http_code = 0;                     // HTTP status code

  // In windows, this will init the winsock stuff
  curl_global_init(CURL_GLOBAL_DEFAULT);

  // Set the headers to JSON, make sure to use UTF-8
  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Accept: application/json");
  headers = curl_slist_append(headers, "Accept-Charset: utf-8");
  headers = curl_slist_append(headers, "charsets: utf-8");
  headers = curl_slist_append(headers, "Content-Type: application/json");

  // get a curl handle
  curl = curl_easy_init();

  if (curl) {
    // Set the URL that we will be using for our POST.
    curl_easy_setopt(curl, CURLOPT_URL, upload_URL.c_str());

    // This is necessary! As I had issues with only 1 byte being sent off
    // to iSENSE unless I made sure to make a string out of the upload_data
    // picojson object, then with that string you can call c_str() on it below.
    std::string upload_real = (value(upload_data).serialize());

    // POST data. Upload will be the string with all the data.
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, upload_real.c_str());

    // JSON Headers
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Verbose debug output - turn this on if you are having problems.
    // It will spit out a ton of information, such as bytes sent off,
    // headers/access/etc. Useful to see if you formatted the data right.

    // *****************
    // Should make a function to turn this off or on
    // or a variable! (bool)
    // *****************

    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    std::cout << "\nrSENSE response: \n";

    // Perform the request, res will get the return code
    curl_easy_perform(curl);

    // This will put the HTTP response code into the "http_code" variable.
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    // Clean up curl.
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    // Return the HTTP code we get from curl.
    return http_code;
  }

  // If curl fails for some reason, return CURL_ERROR (-1).
  return CURL_ERROR;
}


// Checks to see if the given project has been properly setup.
// Shouldn't be any empty values, such as project ID, contributor key, etc.
bool iSENSE::empty_project_check(int type, std::string method) {
  // Check email based values, such as email & password.
  if (type == POST_EMAIL || type == APPEND_EMAIL) {
    if (email == EMPTY || email.empty()) {
      std::cerr << "\nError in method: " << method << "\n";
      std::cerr << "Please set an email address!\n";
      return false;
    }
    if (password == EMPTY || password.empty()) {
      std::cerr << "\nError in method: " << method << "\n";
      std::cerr << "Please set a password!\n";
      return false;
    }
  }

  // Check key based values, such as contributor key and label.
  if (type == POST_KEY || type == APPEND_KEY) {
    if (contributor_key == EMPTY || contributor_key.empty()) {
      std::cerr << "\nError in method: " << method << "\n";
      std::cerr << "Please set a contributor key!\n";
      return false;
    }
    if (contributor_label == "label" || contributor_label.empty()) {
      // If a label wasn't set, automatically set it to "cURL"
      contributor_label = "cURL";
    }
  }

  // The rest are general checks that should not be empty, since the calling
  // method depends on them being set properly.
  if (project_ID == EMPTY || project_ID.empty()) {
    std::cerr << "\nError in method: " << method << "\n";
    std::cerr << "Please set a project ID!\n";
    return false;
  }
  if (title == EMPTY || title.empty()) {
    std::cerr << "\nError in method: " << method << "\n";
    std::cerr << "Please set a project title!\n";
    return false;
  }

  if (map_data.empty()) {
    std::cerr << "\nError in method: " << method << "\n";
    std::cerr << "Map of keys/data is empty.\n";
    std::cerr << "You should push some data back to this object.\n";
    return false;
  }

  // If we don't trigger any of the above if statements, then this project is
  // not empty, so return true.
  return true;
}


// Checks a given HTTP code for errors.
bool iSENSE::check_http_code(int http_code, std::string method) {
  std::cerr << "\nError in method: " << method << "\n";
  std::cerr << "POST request **failed**\n";
  std::cerr << "HTTP Response Code was: " << http_code << "\n";

  if (http_code == 401) {
    std::cerr << "Try checking to make sure your contributor key is valid\n";
    std::cerr << "for the project you are trying to contribute to.\n";
  }
  if (http_code == 404) {
    std::cerr << "Unable to find that project ID.\n";
  }
  if (http_code == 422) {
    std::cerr << "Something went wrong with iSENSE.\n";
    std::cerr << "Try formatting your data differently, using a contributor \n";
    std::cerr << "key instead of an email, or asking for help from others. \n";
    std::cerr << "You can also try running the the program with the debug \n";
    std::cerr << "method enabled, by typing: object_name.debug()\n";
    std::cerr << "This will output a ton of data to the console and \n";
    std::cerr << "may help you in debugging your program.\n";
  }
  if (http_code == CURL_ERROR) {
    std::cerr << "Curl failed for some unknown reason.\n";
    std::cerr << "Make sure you've installed curl / libcurl, and have the \n";
    std::cerr << "picojson header file as well.\n";
  }

  return true;
}


// Call this function to dump all the data in the given object.
void iSENSE::debug() {
  std::cout << "\nProject Title: " << title << "\n";
  std::cout << "Project ID: " << project_ID << "\n";
  std::cout << "Dataset ID: " << dataset_ID << "\n";
  std::cout << "Contributor Key: " << contributor_key << "\n";
  std::cout << "Contributor Label: " << contributor_label << "\n";
  std::cout << "Email Address: " << email << "\n";
  std::cout << "Password: " << password << "\n";
  std::cout << "Upload URL: " << upload_URL << "\n";
  std::cout << "GET URL: " << get_URL << "\n";
  std::cout << "GET User URL: " << get_UserURL << "\n\n";

  std::cout << "Upload string (picojson object): \n";
  std::cout << value(upload_data).serialize() << "\n\n";

  std::cout << "Upload Fields Data (picojson object) \n";
  std::cout << value(fields_data).serialize() << "\n\n";

  std::cout << "GET Data (picojson value): \n";
  std::cout << get_data.serialize() << "\n\n";

  std::cout << "GET Field Data (picojson value): \n";
  std::cout << fields.serialize() << "\n\n";

  std::cout << "GET Fields array (picojson array): \n";
  std::cout << value(fields_array).serialize() << "\n\n";

  // This part may get huge depending on the project!
  // May want a confirm if the user actually wants to display these.
  std::cout << "Datasets (picojson array): ";
  std::cout << value(data_sets).serialize() << "\n\n";

  std::cout << "Media objects (picojson array): ";
  std::cout << value(media_objects).serialize() << "\n\n";

  std::cout << "Owner info (picojson object): ";
  std::cout << value(owner_info).serialize() << "\n\n";

  std::cout << "Map data: \n\n";

  // These for loops will dump all the data in the map.
  // Good for debugging.
  std::map<std::string, std::vector<std::string>>::iterator it;
  std::vector<std::string>::iterator vect;
  for (it = map_data.begin(); it != map_data.end(); it++) {
        std::cout << it->first << " ";

        for (vect = (it->second).begin(); vect != (it->second).end(); vect++) {
              std::cout << *vect << " ";
        }
        std::cout << "\n";
  }
}


//****************************************************************************
// These are needed for picojson & libcURL.
// Declared in memfile.h but defined below.
MEMFILE*  memfopen() {
  MEMFILE* mf = (MEMFILE*) malloc(sizeof(MEMFILE));
  mf->data = NULL;
  mf->size = 0;
  return mf;
}


void memfclose(MEMFILE* mf) {
  // Double check to make sure that mf exists.
  if (mf == NULL) {
    return;
  }

  // OK to free the char array
  if (mf != NULL && mf->data) {
    free(mf->data);
  }

  // And OK to free the structure
  free(mf);
}


// Simple function only used by the get_check_user function to
// suppress curl's output to the screen.
size_t suppress_output(char* ptr, size_t size, size_t nmemb, void* stream) {
  return size * nmemb;
}


size_t memfwrite(char* ptr, size_t size, size_t nmemb, void* stream) {
  MEMFILE* mf = (MEMFILE*) stream;
  int block = size * nmemb;

  if (!mf->data) {
    mf->data = (char*) malloc(block);
  } else {
    mf->data = (char*) realloc(mf->data, mf->size + block);
  }

  if (mf->data) {
    memcpy(mf->data + mf->size, ptr, block);
    mf->size += block;
  }

  return block;
}


char* memfstrdup(MEMFILE* mf) {
  char* buf = (char*)malloc(mf->size + 1);
  memcpy(buf, mf->data, mf->size);
  buf[mf->size] = 0;

  return buf;
}
