/*
 * MIT License
 *
 * Copyright (c) 2020 International Business Machines
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// This is a sample program for education purposes only.
// It implements a very simple homomorphic encryption based
// db search algorithm for demonstration purposes.

// This country lookup example is derived from the BGV database demo
// code originally written by Jack Crawford for a lunch and learn
// session at IBM Research (Hursley) in 2019.

// See more information about this demo in the readme file.

#include <iostream>

#include "helayers/hebase/hebase.h"
#include "helayers/hebase/openfhe/OpenFheBgvContext.h"
#include "helayers/hebase/openfhe/OpenFheDcrtEncoder.h"
#include "helayers/hebase/openfhe/OpenFheDcrtCiphertext.h"
#include "helayers/math/MathUtils.h"
#include <fstream>

using namespace helayers;
using namespace std;

// Forward declarations. These functions are explained later.
vector<pair<string, string>> read_csv(const string& filename, int maxLen);
void run(HeContext& he,
         const string& db_filename,
         const std::string& countryName,
         bool debug,
         int plaintextModulus);
vector<int> stringToAscii(const string& val);
void usage();

int main(int argc, char* argv[])
{
  // Note: The parameters have been chosen to provide a somewhat
  // faster running time with a non-realistic security level.
  // Do Not use these parameters in real applications.

  // input database file name
  string db_filename = getDataSetsDir() + "/countries/countries.csv";
  // debug output (default no debug output)
  bool debug = false;

  int plaintextModulus = 257; // 786433;

  string countryName = "";

  int i = 1;
  while (i < argc) {
    string arg = argv[i++];
    if (arg == "--help") {
      usage();
      return 0;
    }
    if (arg == "--plaintext_modulus")
      plaintextModulus = atoi(argv[i++]);
    else if (arg == "--db_filename")
      db_filename = argv[i++];
    else if (arg == "--country")
      countryName = argv[i++];
    else if (arg == "--debug")
      debug = true;
    else
      throw runtime_error("Unsupported argument: " + arg);
  }

  cout << "\n*********************************************************";
  cout << "\n*           Privacy Preserving Search Example           *";
  cout << "\n*           =================================           *";
  cout << "\n*                                                       *";
  cout << "\n* This is a sample program for education purposes only. *";
  cout << "\n* It implements a very simple homomorphic encryption    *";
  cout << "\n* based db search algorithm for demonstration purposes. *";
  cout << "\n*                                                       *";
  cout << "\n*********************************************************";
  cout << endl;

  cout << "---Initialising HE Environment ... ";
  // Initialize context
  cout << "\nInitializing the Context ... " << endl;

  // Next we'll initialize a BGV scheme in openFHE.
  // The following lines perform full initialization
  // Including key generation.
  // (We added code for timing it).
  OpenFheBgvContext he;
  cout << "initializing he..." << endl;

  // Since we store ascii codes, we need it at least to be able
  // to handle the numbers 0...127
  always_assert(plaintextModulus >= 127);

  HeConfigRequirement req = HeConfigRequirement::insecure(32, 16);
  req.plaintextModulus = plaintextModulus;
  HELAYERS_TIMER_PUSH("Initialization");

  he.init(req);
  HELAYERS_TIMER_POP();

  // OpenFHE-BGV is now ready to start doing some HE work.
  // which we'll do in the following function, defined below
  run(he, db_filename, countryName, debug, plaintextModulus);

  return 0;
}

void usage()
{
  cout << "Usage:" << endl;
  cout << endl;
  cout << "\t--plaintext_modulus\t\tPlaintext modulus" << endl;
  cout << "\t--db_filename\t\t\tQualified name for the database filename"
       << endl;
  cout << "\t--country <int>\t\t\tCountry to search for" << endl;
  cout << "\t---debug\t\t\tDebug" << endl;
  cout << endl;
}

void pow(HeContext& he, CTile& ctile, int degree)
{
  bool yIsOne = true;
  CTile y(he);
  CTile x = ctile;

  while (degree > 1) {
    if ((degree % 2) == 0) {
      ctile.square();
      degree = degree / 2;
    } else {
      if (yIsOne) {
        y = ctile;
        yIsOne = false;
      } else {
        y.multiply(ctile);
      }
      ctile.square();
      degree = (degree - 1) / 2;
    }
  }
  if (!yIsOne)
    ctile.multiply(y);
}

void run(HeContext& he,
         const string& db_filename,
         const std::string& countryName,
         bool debug,
         int plaintextModulus)
{

  // The run function receives an abstract HeContext class.
  // Therefore the code below is oblivious to a particular HE scheme
  // implementation.

  // First let's print general information on our library and scheme.
  // This will print their names, and the configuration details.
  he.printSignature();

  // However we do have some requirements that we can

  always_assert(he.getTraits().isModularArithmetic());

  // Next, print the security level
  // Note: This will be negligible to improve performance time.
  cout << "\n***Security Level: " << he.getSecurityLevel()
       << " *** Negligible for this example ***" << endl;

  // Let's also print the number of slots.
  // Each ciphertext will have this many slots.
  cout << "\nNumber of slots: " << he.slotCount() << endl;

  // Now we'll read in the database (in cleartext).
  // This function we'll make sure no string is longer than he.slotCount()
  vector<pair<string, string>> country_db =
      read_csv(db_filename, he.slotCount());

  cout << "\n---Initializing the encrypted key,value pair database ("
       << country_db.size() << " entries)...";
  cout << "\nConverting strings to numeric representation into Ptxt objects "
          "..."
       << endl;

  // We'll now encrypt our country-capital database.
  HELAYERS_TIMER_PUSH("CountryDB");
  // The encoder class handles both encoding and encrypting.
  Encoder enc(he);
  // This is the database: a vector of pairs of CTile-s.
  // A CTile is a ciphertext object.
  vector<pair<CTile, CTile>> encrypted_country_db;
  for (const auto& country_capital_pair : country_db) {
    // Create a country ciphertext, and encrypt inside
    // the ascii vector representation of each country.
    // For example, Norway is represented
    // (78,111,114,119,97,121,  0,0,0, ...)
    CTile country(he);
    enc.encodeEncrypt(country, stringToAscii(country_capital_pair.first));
    // Similarly encrypt the capital name
    CTile capital(he);
    enc.encodeEncrypt(capital, stringToAscii(country_capital_pair.second));
    // Add the pair to the database

    vector<int> resInt = enc.decryptDecodeInt(country);
    encrypted_country_db.emplace_back(std::move(country), std::move(capital));
  }
  HELAYERS_TIMER_POP();

  cout << "\nInitialization Completed - Ready for Queries" << endl;
  cout << "--------------------------------------------" << endl;

  /** Create the query **/

  // Read in query from the command line
  string query_string;
  if (countryName == "") {
    cout << "\nPlease enter the name of a Country: ";
    getline(cin, query_string);
  } else
    query_string = countryName;

  cout << "Looking for the Capital of " << query_string << endl;
  cout << "This may take a few minutes ... " << endl;

  HELAYERS_TIMER_PUSH("TotalQuery");
  HELAYERS_TIMER_PUSH("EncryptQuery");

  // Encrypt the query similar to the way we encrypted
  // the country and capital names
  CTile query(he);
  enc.encodeEncrypt(query, stringToAscii(query_string));

  HELAYERS_TIMER_POP();

  /************ Perform the database search ************/

  HELAYERS_TIMER_PUSH("QuerySearch");
  vector<CTile> mask;
  mask.reserve(country_db.size());

  // For every entry in our database we perform the following
  // calculation:
  for (const auto& encrypted_pair : encrypted_country_db) {
    //  Copy of database key: a country name
    CTile mask_entry = encrypted_pair.first;
    // Calculate the difference
    // In each slot now we'll have 0 when characters match,
    // or non-zero when there's a mismatch

    mask_entry.sub(query);

    // Fermat's little theorem:
    // Since the underlying plaintext are in modular arithmetic,
    // Raising to the power of modulusP- 1 converts all non-zero values
    // to 1.

    CTile res = mask_entry;
    pow(he, res, plaintextModulus - 1);

    // Negate the ciphertext
    // Now we'll have 0 for match, -1 for mismatch
    res.negate();

    // Add +1
    // Now we'll have 1 for match, 0 for mismatch

    vector<int> valsOne = vector<int>(he.slotCount(), 1);
    CTile one(he);
    enc.encodeEncrypt(one, valsOne);
    res.add(one);

    // We'll now multiply all slots together, since
    // we want a complete match across all slots.

    // If slot count is a power of 2 (our case 32) there's an efficient way
    // to do it:
    // we'll do a rotate-and-multiply algorithm, similar to
    // a rotate-and-sum one.

    for (int rot = 1; rot < he.slotCount(); rot *= 2) {
      CTile tmp(res);
      tmp.rotate(-rot);
      res.multiply(tmp);
    }

    // mask_entry is now either all 1s if query==country,
    // or all 0s otherwise.
    // After we multiply by capital name it will be either
    // the capital name, or all 0s.
    res.multiply(encrypted_pair.second);

    // We collect all our findings.
    mask.push_back(res);
  }
  HELAYERS_TIMER_POP();

  // Aggregate the results into a single ciphertext
  // Note: This code is for educational purposes and thus we try to refrain
  // from using the STL and do not use std::accumulate
  CTile value = mask[0];
  for (int i = 1; i < mask.size(); i++)
    value.add(mask[i]);

  // /************ Decrypt and print result ************/

  HELAYERS_TIMER_PUSH("DecryptQueryResult");
  vector<int> res = enc.decryptDecodeInt(value);
  HELAYERS_TIMER_POP();

  // Convert from ASCII to a string
  string string_result;
  for (long i = 0; i < res.size(); ++i)
    string_result.push_back(static_cast<long>(res[i]));

  HELAYERS_TIMER_POP();

  if (string_result.at(0) == 0x00) {
    string_result = "Country name not in the database.\n*** Please make sure "
                    "to enter the name of an European Country\n*** with the "
                    "first letter in upper case.";
  }
  if (debug)
    HELAYERS_TIMER_PRINT_MEASURES_SUMMARY_FLAT();
  cout << "\nQuery result: " << string_result << endl;
}

// Utility function to read <K,V> CSV data from file
vector<pair<string, string>> read_csv(const string& filename, int maxLen)
{
  vector<pair<string, string>> dataset;
  ifstream data_file =
      FileUtils::openIfstream(filename, ios_base::in, ifstream::badbit);

  if (!data_file.is_open())
    throw runtime_error(
        "Error: This example failed trying to open the data file: " + filename +
        "\n           Please check this file exists and try again.");

  vector<string> row;
  string line, entry, temp;

  if (data_file.good()) {
    // Read each line of file
    while (getline(data_file, line)) {
      row.clear();
      stringstream ss(line);
      while (getline(ss, entry, ',')) {
        row.push_back(entry);
      }
      if (row[0].size() > maxLen)
        throw runtime_error("Country name " + row[0] + " too long");
      if (row[1].size() > maxLen)
        throw runtime_error("Capital name " + row[1] + " too long");

      // Add key value pairs to dataset
      dataset.push_back(make_pair(row[0], row[1]));
    }
  }

  data_file.close();
  return dataset;
}

// Return a vector of ints with the i'th element containing the ascii
// code of the i'th character
vector<int> stringToAscii(const string& val)
{
  vector<int> res;
  res.reserve(val.size());
  for (size_t i = 0; i < val.size(); ++i) {
    res.push_back(val[i]);
  }
  return res;
}
