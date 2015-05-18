#include "include/API.h"

// For picojson
using namespace picojson;

/* This file requires Boost. Please make sure to run the following command
 * before compiling:
 * sudo apt-get install libboost-test-dev
 * The above command will install Boost into your development environment.
 */
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Main
#include <boost/test/unit_test.hpp>

/* This file will run a series of tests on the C++ API
 * The following methods are tested:
 *
 * get_check_user()
 * get_project_fields()
 * get_datasets_and_mediaobjects()
 * get_projects_search()
 * get_dataset()
 * get_dataset_ID()
 * get_field_ID()
 * post_json_email()
 * post_json_key()
 * append_email_byID()
 * append_email_byName()
 * append_key_byID()
 * append_key_byName()
 *
 */

const std::string PROJ_ID = "1006";
const std::string TITLE = "TITLE";
const std::string LABEL = "Boost";
const std::string KEY = "123";

const std::string DATASET_ID = "2801";
const std::string FIELD_ID = "681";

/*
 * This is a derived class to quickly test the append_byID functions.
 * It is derived from iSENSE, and by doing this I can create a public function
 * which can access the two append_byID functions publicly.
 * Users would have to derive the class to do the same - simply using iSENSE,
 * the append_byID functions will be protected and unaccessable (since they
 * should be using the "append_byName, using dataset names rather than IDs)
 */
class Test: public iSENSE {
 public:
  void check_set_dataset_ID(std::string proj_dataset_ID) {
    set_dataset_ID(proj_dataset_ID);
  }
  bool check_append_key_byID(std::string dataset_ID) {
    return append_key_byID(dataset_ID);
  }
  bool check_append_email_byID(std::string dataset_ID) {
    return append_email_byID(dataset_ID);
  }
};


// Ensure the CheckUser() method works.
BOOST_AUTO_TEST_CASE(get_check_user) {
  iSENSE test;

  // Add test project
  test.set_project_ID(PROJ_ID);
  test.set_project_title("get check User " + test.generate_timestamp());

  // This is a valid email / password - so it should return true.
  BOOST_REQUIRE(test.set_email_password("j@j.j", "j") == true);
}


// Test the GET project fields method.
BOOST_AUTO_TEST_CASE(get_project_fields) {
  iSENSE test_true(PROJ_ID, TITLE, LABEL, KEY);

  // Make the title unique
  test_true.set_project_title("get project fields" + test_true.generate_timestamp());

  // Test the GET method, should work for this project ID
  BOOST_REQUIRE(test_true.get_project_fields() == true);

  iSENSE test_false;

  // This should fail.
  BOOST_REQUIRE(test_false.get_project_fields() == false);
}


// Test the get_datasets / media objects method
BOOST_AUTO_TEST_CASE(get_datasets_and_mediaobjects) {
  iSENSE test_true(PROJ_ID, TITLE, LABEL, KEY);

  // Make the title unique
  test_true.set_project_title("get datasets and media objects " + test_true.generate_timestamp());

  // Test the GET method, should work for this project ID
  BOOST_REQUIRE(test_true.get_datasets_and_mediaobjects() == true);

  iSENSE test_false;

  // This should fail.
  BOOST_REQUIRE(test_false.get_datasets_and_mediaobjects() == false);
}


// Test the search project method
BOOST_AUTO_TEST_CASE(get_projects_search) {
  iSENSE test_true(PROJ_ID, TITLE, LABEL, KEY);

  std::string search_term = "test";

  std::vector<std::string> project_titles;
  project_titles = test_true.get_projects_search(search_term);

  BOOST_REQUIRE(project_titles.empty() == false);

  iSENSE test_false;

  search_term = EMPTY;

  project_titles.clear();
  project_titles = test_false.get_projects_search(search_term);

  BOOST_REQUIRE(project_titles.empty() == true);
}


// Test the get dataset method
BOOST_AUTO_TEST_CASE(get_dataset) {
  iSENSE test_true("106", "test", "BOOST Test", "123");

  std::vector<std::string> baseball_hits;
  baseball_hits = test_true.get_dataset("MLB Team Statistics 2013", "Runs");

  BOOST_REQUIRE(baseball_hits.empty() == false);

  iSENSE test_false;

  baseball_hits.clear();
  baseball_hits = test_false.get_dataset("MLB Team Statistics 2013", "Runs");

  BOOST_REQUIRE(baseball_hits.empty() == true);
}


// Test the get_dataset_ID method
BOOST_AUTO_TEST_CASE(get_dataset_ID) {
  iSENSE test_true("106", "test", "BOOST Test", "123");

  // Pull down datasets
  test_true.get_datasets_and_mediaobjects();

  // Try and get dataset ID for the given dataset name.
  std::string datasetID = test_true.get_dataset_ID("MLB Team Statistics 2013");

  BOOST_REQUIRE(datasetID == DATASET_ID);

  iSENSE test_false;

  // Not pulling down datasets (or even setting a project ID!)
  // So this should be equal to GET_ERROR
  datasetID = test_false.get_dataset_ID("MLB Team Statistics 2013");

  BOOST_REQUIRE(datasetID == GET_ERROR);
}


// Test the get_field_ID method
BOOST_AUTO_TEST_CASE(get_field_ID) {
  iSENSE test_true("106", "test", "BOOST Test", "123");

  // Pull down fields
  test_true.get_project_fields();

  // Try and get a field ID for the given field.
  std::string fieldID = test_true.get_field_ID("Wins");

  BOOST_REQUIRE(fieldID == "637");

  iSENSE test_false;

  // Not pullin down fields, or setting a project ID, so this should be
  // equal to GET_ERROR.
  fieldID = test_false.get_field_ID("Wins");

  BOOST_REQUIRE(fieldID == GET_ERROR);
}


// Test POSTing with Contributor keys
BOOST_AUTO_TEST_CASE(post_JSON_withKey) {
  iSENSE test(PROJ_ID, TITLE, LABEL, KEY);

  // Make the title unique
  test.set_project_title("POST JSON with Keys " + test.generate_timestamp());

  // Push some data back
  test.push_back("Timestamp", test.generate_timestamp());
  test.push_back("Number", "123");
  test.push_back("Text", "ABC");

  BOOST_REQUIRE(test.post_json_key() == true);
}


// Test POSTing with Email/Password
BOOST_AUTO_TEST_CASE(post_JSON_withEmail) {
  iSENSE test(PROJ_ID, TITLE, LABEL, KEY);

  // Make the title unique + set email/password
  test.set_project_title("POST JSON with Email " + test.generate_timestamp());
  test.set_email_password("j@j.j", "j");

  // Push some data back
  test.push_back("Timestamp", test.generate_timestamp());
  test.push_back("Number", "123");
  test.push_back("Text", "ABC");

  BOOST_REQUIRE(test.post_json_email() == true);
}


// Test Appending with Dataset IDs
// (Email / Password)
BOOST_AUTO_TEST_CASE(append_withDatasetID_byEmail) {
  Test test;
  std::string title, ID, dataset_ID, email, password, letters, num, timestamp;

  // This will be a test of the append method.
  ID = PROJ_ID;
  title = "Testing";
  email = "j@j.j";
  password = "j";
  dataset_ID = "8715";

  // Add project info / dataset info to the object
  test.set_project_ID(ID);
  test.check_set_dataset_ID(dataset_ID);
  test.set_project_title(title);
  test.set_email_password(email, password);

  test.push_back("Number", "123456789");
  test.push_back("Text", "THIS");
  test.push_back("Timestamp", test.generate_timestamp());

  BOOST_REQUIRE(test.check_append_email_byID(dataset_ID) == true);
}


// Test Appending with Dataset names
// (Email / Password)
BOOST_AUTO_TEST_CASE(append_withDatasetName_byEmail) {
  iSENSE test;
  std::string title, ID, dataset_name, email, password, letters, num, timestamp;

  // This will be a test of the append method.
  ID = PROJ_ID;
  title = "Testing";
  email = "j@j.j";
  password = "j";
  dataset_name = "Testing";

  // Add project info / dataset info to the object
  test.set_project_ID(ID);
  test.set_project_title(title);
  test.set_email_password(email, password);

  // Push data back to the object.
  test.push_back("Number", "123456789");
  test.push_back("Text", "THIS");
  test.push_back("Timestamp", test.generate_timestamp());

  BOOST_REQUIRE(test.append_email_byName(dataset_name) == true);
}


// Test Appending with Dataset IDs
// (Contributor keys)
BOOST_AUTO_TEST_CASE(append_withDatasetID_byKey) {
  Test test;
  std::string title, ID, datasetID, key, letters, num, timestamp;

  // This will be a test of the append method.
  title = "Testing";
  ID = PROJ_ID;
  key = "123";
  datasetID = "8715";

  // Add project info / dataset info to the object
  test.set_project_ID(ID);
  test.check_set_dataset_ID(datasetID);
  test.set_project_title(title);
  test.set_contributor_key(key);

  timestamp = test.generate_timestamp();

  test.push_back("Number", "999999");

  BOOST_REQUIRE(test.check_append_key_byID(datasetID) == true);
}


// Test Appending with Dataset names
// (Contributor keys)
BOOST_AUTO_TEST_CASE(append_withDatasetName_byKey) {
  iSENSE test;
  std::string title, ID, dataset_name, label, key;

  // This will be a test of the append method.
  ID = PROJ_ID;
  title = "test";
  label = "BOOST Test";
  key = "123";
  dataset_name = "Testing";

  // Add project info / dataset info to the object
  test.set_project_ID(ID);
  test.set_project_title(title);
  test.set_project_label(label);
  test.set_contributor_key(key);

  // Push data back to the object.
  test.push_back("Number", "123456789");
  test.push_back("Text", "THIS");
  test.push_back("Timestamp", test.generate_timestamp());

  BOOST_REQUIRE(test.append_key_byName(dataset_name) == true);
}

