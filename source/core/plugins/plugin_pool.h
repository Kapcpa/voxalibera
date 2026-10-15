#pragma once

#include "core/plugins/plugin_library.h"
#include "core/project/model.h"
#include "engine/plugins/plugin_chain.h"

#include <map>
#include <memory>

class PluginPool
{
public:
	explicit PluginPool(PluginLibrary& library);

	void use_rate(double sample_rate);
	void forget_removed(const Project& project);
	std::shared_ptr<PluginChain> chain_for(const Track& track);
	std::shared_ptr<PluginProcessor> find_instance(PluginId id) const;
	bool is_missing(PluginId id) const;
	bool take_changes();
	void retry_missing();
	int revision() const { return availability_revision; }
	void store_states(Project& project) const;
	void clear();

private:
	struct Instance
	{
		std::shared_ptr<PluginProcessor> processor;
		int seen_changes = 0;
	};

	std::shared_ptr<PluginProcessor> instance_for(const Plugin& plugin);
	std::shared_ptr<PluginProcessor> create_instance(const Plugin& plugin);
	std::shared_ptr<PluginProcessor> create_hosted(const Plugin& plugin);
	void log_plugin_problem(const Plugin& plugin, const juce::String& problem) const;
	void retire(PluginId id);

	PluginLibrary& library;
	double sample_rate = 0.0;
	int availability_revision = 0;
	std::map<TrackId, std::shared_ptr<PluginChain>> chains;
	std::map<PluginId, Instance> instances;
	std::map<PluginId, juce::MemoryBlock> retired_states;
};
