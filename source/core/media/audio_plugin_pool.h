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

	void remove_track(TrackId track_id);
	void clear();

	void prepare_all(const PluginProcessContext& context);

private:
	std::map<TrackId, std::shared_ptr<PluginChain>> chains;
};