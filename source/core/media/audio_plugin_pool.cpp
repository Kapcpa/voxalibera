#include "core/media/audio_plugin_pool.h"

std::shared_ptr<PluginChain> AudioPluginPool::get_chain(TrackId track_id) const
{
	auto it = chains.find(track_id);
	return (it != chains.end()) ? it->second : nullptr;
}

std::shared_ptr<PluginChain> AudioPluginPool::obtain_chain(TrackId track_id)
{
	auto& chain = chains[track_id];
	if (!chain)
		chain = std::make_shared<PluginChain>();
	return chain;
}

void AudioPluginPool::remove_track(TrackId track_id)
{
	chains.erase(track_id);
}

void AudioPluginPool::clear()
{
	chains.clear();
}

void AudioPluginPool::prepare_all(const PluginProcessContext& context)
{
	for (auto& [id, chain] : chains)
		if (chain)
			chain->prepare(context);
}