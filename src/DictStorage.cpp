/*
 * Open Chinese Convert
 *
 * Copyright 2013-2013 BYVoid <byvoid@byvoid.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "DictStorage.hpp"
#include "TextDict.hpp"
#include "UTF8Util.hpp"

using namespace Opencc;

const char* OCDHEADER = "OPENCCDICTIONARY1";

void DictStorage::serialize(TextDict& dictionary, const string fileName) {
  getLexicon(dictionary);
  buildDarts();
  writeToFile(fileName);
}

void DictStorage::getLexicon(TextDict& dictionary) {
  vector<TextDict::TextEntry> tlexicon = dictionary.GetLexicon();
  size_t lexicon_count = tlexicon.size();
  size_t lexicon_cursor = 0;
  lexicon.resize(lexicon_count);
  for (size_t i = 0; i < lexicon_count; i++) {
    lexicon[i].key = tlexicon[i].key;
    size_t value_count = tlexicon[i].values.size();
    for (size_t j = 0; j < value_count; j++) {
      lexicon[i].valueIndexes.push_back(values.size());
      values.push_back(Value(tlexicon[i].values[j], lexicon_cursor));
      lexicon_cursor += tlexicon[i].values[j].length();
    }
  }
}

void DictStorage::buildDarts() {
  vector<const char*> keys;
  vector<int> valueIndexes;
  keys.reserve(lexicon.size());
  for (size_t i = 0; i < lexicon.size(); i++) {
    keys.push_back(lexicon[i].key.c_str());
  }
  dict.build(lexicon.size(), &keys[0]);
}

void DictStorage::writeToFile(const string fileName) {
  FILE *fp = fopen(fileName.c_str(), "wb");
  if (fp == NULL) {
    fprintf(stderr, _("Can not write file: %s\n"), fileName.c_str());
    exit(1);
  }
  /*
   Binary Structure
   [OCDHEADER]
   [number of keys]
   size_t
   [number of values]
   size_t
   [bytes of values]
   size_t
   [bytes of darts]
   size_t
   [key indexes]
   size_t(index of first value) * [number of keys]
   [value indexes]
   size_t(index of first char) * [number of values]
   [value data]
   char * [bytes of values]
   [darts]
   char * [bytes of darts]
   */
  size_t numKeys = lexicon.size();
  size_t numValues = values.size();
  size_t dartsSize = dict.total_size();
  
  fwrite(OCDHEADER, sizeof(char), strlen(OCDHEADER), fp);
  fwrite(&numKeys, sizeof(size_t), 1, fp);
  fwrite(&numValues, sizeof(size_t), 1, fp);
  fwrite(&dartsSize, sizeof(size_t), 1, fp);
  
  // key indexes
  for (size_t i = 0; i < numKeys; i++) {
    size_t index = lexicon[i].valueIndexes[0];
    fwrite(&index, sizeof(size_t), 1, fp);
  }
  
  // value indexes
  for (size_t i = 0; i < numValues; i++) {
    size_t index = values[i].cursor;
    fwrite(&index, sizeof(size_t), 1, fp);
  }
  
  // value data
  for (size_t i = 0; i < numValues; i++) {
    const char* value = values[i].value.c_str();
    fwrite(value, sizeof(char), strlen(value), fp);
  }
  
  // darts
  fwrite(dict.array(), sizeof(char), dartsSize, fp);
  
  fclose(fp);
}
