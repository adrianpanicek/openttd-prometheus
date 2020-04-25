/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2. OpenTTD is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details. You should have received a copy of the GNU
 * General Public License along with OpenTTD. If not, see
 * <http://www.gnu.org/licenses/>.
 */

/** @file metrics.h Header file for prometheus metrics */

#ifndef METRICS_H
#define METRICS_H

#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

#include "stdafx.h"
#include "cargotype.h"
#include "economy_type.h"
#include "group.h"
#include "vehicle_type.h"

namespace prom {
using namespace prometheus;

extern Family<Counter>& cash_counter_family;

void RegisterMetrics();

class CompanyMetrics {
 private:
  std::string name;

  std::map<std::pair<CargoLabel, VehicleType>,
           std::shared_ptr<prometheus::Counter>>
      cargo_delivered_counters;

  std::map<std::pair<CargoLabel, VehicleType>,
           std::shared_ptr<prometheus::Counter>>
      cargo_delivered_income_counters;

  std::map<VehicleType, std::shared_ptr<prometheus::Gauge>>
      vehicles_owned_family_gauges;

  std::map<ExpensesType, std::shared_ptr<prometheus::Counter>>
      vehicle_running_costs_family_counters;

  std::shared_ptr<prometheus::Gauge> bank_balance;

 public:
  CompanyMetrics(char* name);
  ~CompanyMetrics();

  std::shared_ptr<prometheus::Counter> income_counter;
  std::shared_ptr<prometheus::Counter> expenses_counter;
  std::shared_ptr<prometheus::Counter> trees_planted_expenses_counter;

  void increment_cargo_delivered(CargoLabel label, VehicleType type,
                                 double amount);
  void increment_cargo_delivered_income(CargoLabel label, VehicleType type,
                                        double amount);

  void increment_vehicle_running_costs(ExpensesType type, double amount);

  void update_vehicle_counts(GroupStatistics* gs);

  void update_bank_balance(Money money, Money current_loan);

  std::string get_company_name();
};

using namespace std;

struct Metric {
    const shared_ptr<string> name;
    const shared_ptr<string> description;

    Metric(
        const shared_ptr<string> _name, 
        const shared_ptr<string> _description
    ) : name{_name}, description{_description} {};
};

struct Label {
    const string name;
    const string value;
};

template<typename T, T initial>
struct ScalarMetric : public Metric {
    const vector<Label> labels;
    T value {initial};

    template<size_t C>
    ScalarMetric(
        const shared_ptr<string> _name, 
        const shared_ptr<string> _description, 
        const vector<Label> _labels
    ) : Metric(_name, _description), labels{_labels} {};

    ~ScalarMetric() {
        delete labels;
    }
};

struct CounterMetric : public ScalarMetric<unsigned long, 0UL> {
    void increase(unsigned long by = 1) {
        value += by;
    }
};

class GaugeMetric : public ScalarMetric<long, 0L> {
    void increase(unsigned long by = 1) {
        value += by;
    }

    void decrease(unsigned long by = 1) {
        value -= by;
    }
};

using BucketLabel = std::string;
using BucketValue = unsigned long;

struct HistogramBucket {
    const BucketLabel label;
    const BucketValue value;
};

class HistogramMetric : public Metric {
    private: 
        BucketValue lastSum = 0;
        bool recountSum = true;
    protected:
        vector<HistogramBucket> value {};
    public: 
        void addBucket(HistogramBucket bucket);
        void removeBucket(BucketLabel label);

        BucketValue getSum();
        size_t getCount();
};

class MetricsSerializer {
    private:
        vector<string> printedMeta;
    public:
        template<typename T, T I>
        string MetricsSerializer::serialize(shared_ptr<ScalarMetric<T, I>> metric);

        string serialize(shared_ptr<HistogramMetric> metric);
};


}  // namespace prom

#endif /* METRICS_H */