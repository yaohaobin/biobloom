/*
 * ResultsManager.h
 *
 *  Created on: Jun 25, 2013
 *      Author: cjustin
 */

#ifndef RESULTSMANAGER_H_
#define RESULTSMANAGER_H_

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include "Common/Options.h"
#if _OPENMP
# include <omp.h>
#endif

using namespace std;

static const string NO_MATCH = "noMatch";
static const string MULTI_MATCH = "multiMatch";

template<typename T>
class ResultsManager {
public:
	explicit ResultsManager<T>(const vector<string> &filterOrderRef,
			bool inclusive) :
			m_filterOrder(filterOrderRef), m_multiMatchIndex(
					filterOrderRef.size() + 1), m_aboveThreshold(
					filterOrderRef.size() + 2, 0), m_unique(
					filterOrderRef.size() + 2, 0), m_multiMatch(0), m_noMatch(
					0), m_inclusive(inclusive) {

	}

	void updateSummaryData(T hit) {
		if (hit == 0) {
#pragma omp atomic
			++m_noMatch;
		} else {
#pragma omp atomic
			++m_aboveThreshold[hit];
#pragma omp atomic
			++m_unique[hit];
		}
	}

	T updateSummaryData(const vector<T> &hits) {
		T filterIndex = 0;
		for (typename vector<T>::const_iterator i = hits.begin();
				i != hits.end(); ++i) {
#pragma omp atomic
			++m_aboveThreshold[*i];
			if (filterIndex == 0) {
				filterIndex = *i;
			} else {
				filterIndex = m_multiMatchIndex;
			}
		}
		if (filterIndex == 0) {
#pragma omp atomic
			++m_noMatch;
		} else if (filterIndex == m_multiMatchIndex) {
#pragma omp atomic
			++m_multiMatch;
		} else {
#pragma omp atomic
			++m_unique[filterIndex];
		}
		return filterIndex;
	}

	T updateSummaryData(const vector<T> &hits1, const vector<T> &hits2) {
		unsigned filterIndex = 0;
		typename vector<T>::const_iterator i1 = hits1.begin();
		typename vector<T>::const_iterator i2 = hits2.begin();
		if (m_inclusive) {
			while (i1 != hits1.end() && i2 != hits2.end()) {
				//check if hits are the same
				if (*i1 == *i2) {
					//if they are increment above threshold counts
#pragma omp atomic
					++m_aboveThreshold[*i1];
					//increment both indexes
					if (filterIndex == 0) {
						filterIndex = *i1;
					} else {
						filterIndex = m_multiMatchIndex;
					}
					++i1;
					++i2;
				}
				//if not increment the smaller value index
				else if (*i1 < *i2) {
#pragma omp atomic
					++m_aboveThreshold[*i1];
					if (filterIndex == 0) {
						filterIndex = *i1;
					} else {
						filterIndex = m_multiMatchIndex;
					}
					++i1;
				} else {
#pragma omp atomic
					++m_aboveThreshold[*i2];
					if (filterIndex == 0) {
						filterIndex = *i2;
					} else {
						filterIndex = m_multiMatchIndex;
					}
					++i2;
				}
			}
			//finish off
			while (i1 != hits1.end()) {
#pragma omp atomic
				++m_aboveThreshold[*i1];
				if (filterIndex == 0) {
					filterIndex = *i1;
				} else {
					filterIndex = m_multiMatchIndex;
				}
				++i1;
			}
			while (i2 != hits2.end()) {
#pragma omp atomic
				++m_aboveThreshold[*i2];
				if (filterIndex == 0) {
					filterIndex = *i2;
				} else {
					filterIndex = m_multiMatchIndex;
				}
				++i2;
			}
		} else {
			while (i1 != hits1.end() && i2 != hits2.end()) {
				//check if hits are the same
				if (*i1 == *i2) {
					//if they are increment above threshold counts
#pragma omp atomic
					++m_aboveThreshold[*i1];
					//increment both indexes
					if (filterIndex == 0) {
						filterIndex = *i1;
					} else {
						filterIndex = m_multiMatchIndex;
					}
					++i1;
					++i2;
				}
				//if not increment the smaller value index
				else if (*i1 < *i2) {
					++i1;
				} else {
					++i2;
				}
			}
		}

		if (filterIndex == 0) {
#pragma omp atomic
			++m_noMatch;
		} else if (filterIndex == m_multiMatchIndex) {
#pragma omp atomic
			++m_multiMatch;
		} else {
#pragma omp atomic
			++m_unique[filterIndex];
		}
		return filterIndex;
	}

	const string getResultsSummary(size_t readCount) const {

		stringstream summaryOutput;

		//print header
		summaryOutput
				<< "filter_id\thits\tmisses\tshared\trate_hit\trate_miss\trate_shared\n";

		for (unsigned i = 0; i < m_filterOrder.size(); ++i) {
			summaryOutput << m_filterOrder.at(i);
			summaryOutput << "\t" << m_aboveThreshold.at(i);
			summaryOutput << "\t" << readCount - m_aboveThreshold.at(i);
			summaryOutput << "\t" << (m_aboveThreshold.at(i) - m_unique.at(i));
			summaryOutput << "\t"
					<< double(m_aboveThreshold.at(i)) / double(readCount);
			summaryOutput << "\t"
					<< double(readCount - m_aboveThreshold.at(i))
							/ double(readCount);
			summaryOutput << "\t"
					<< double(m_aboveThreshold.at(i) - m_unique.at(i))
							/ double(readCount);
			summaryOutput << "\n";
		}

		summaryOutput << MULTI_MATCH;
		summaryOutput << "\t" << m_multiMatch;
		summaryOutput << "\t" << readCount - m_multiMatch;
		summaryOutput << "\t" << 0;
		summaryOutput << "\t" << double(m_multiMatch) / double(readCount);
		summaryOutput << "\t"
				<< double(readCount - m_multiMatch) / double(readCount);
		summaryOutput << "\t" << 0.0;
		summaryOutput << "\n";

		summaryOutput << NO_MATCH;
		summaryOutput << "\t" << m_noMatch;
		summaryOutput << "\t" << readCount - m_noMatch;
		summaryOutput << "\t" << 0;
		summaryOutput << "\t" << double(m_noMatch) / double(readCount);
		summaryOutput << "\t"
				<< double(readCount - m_noMatch) / double(readCount);
		summaryOutput << "\t" << 0.0;
		summaryOutput << "\n";

//		cerr << summaryOutput.str() << endl;
		return summaryOutput.str();
	}

	T getMultiMatchIndex() const {
		return m_multiMatchIndex;
	}

	virtual ~ResultsManager() {
	}

private:
	//Variables owned by biobloomcategorizer
	const vector<string> &m_filterOrder;
	const T m_multiMatchIndex;

	vector<size_t> m_aboveThreshold;
	vector<size_t> m_unique;
	size_t m_multiMatch;
	size_t m_noMatch;
	bool m_inclusive;
};

#endif /* RESULTSMANAGER_H_ */