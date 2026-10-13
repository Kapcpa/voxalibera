#pragma once

#include "core/project/model.h"
#include "engine/plugins/plugin_chain.h"
#include <memory>
#include <unordered_map>

class AudioPluginPool
{
public:
	AudioPluginPool() = default;

	std::shared_ptr<PluginChain> get_chain(TrackId track_id) const;
	std::shared_ptr<PluginChain> obtain_chain(TrackId track_id);

	void sync_track_plugins(const Track& track, double sample_rate, int buffer_size);

	void remove_track(TrackId track_id);
	void clear();

	void prepare_all(const PluginProcessContext& context);

private:
	std::shared_ptr<PluginProcessor> create_processor(const Plugin& plugin);

	std::map<TrackId, std::shared_ptr<PluginChain>> chains;
	std::map<PluginId, std::shared_ptr<PluginProcessor>> instances;
};