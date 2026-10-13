#include "core/media/audio_plugin_pool.h"
#include "engine/plugins/test_processor.h"

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

std::shared_ptr<PluginProcessor> AudioPluginPool::create_processor(const Plugin& plugin)
{
	// vst loader will come here later on instead.
	if (plugin.name == "Test Overdrive" || plugin.name == "Drive") {
		return std::make_shared<TestDriveProcessor>();
	}

	return nullptr;
}

void AudioPluginPool::sync_track_plugins(const Track& track, double sample_rate, int buffer_size)
{
	auto chain = obtain_chain(track.id);
	chain->clear();

	PluginProcessContext plugin_context(sample_rate, buffer_size);

	for (size_t i = 0; i < track.plugins.size(); ++i) {
		const auto& plugin_data = track.plugins[i];
		auto& proc = instances[plugin_data.id];

		if (proc == nullptr) {
			proc = create_processor(plugin_data);
			if (proc)
				proc->prepare(plugin_context);
		}

		if (proc != nullptr) {
			chain->add_processor(proc);
			chain->set_bypassed(i, plugin_data.bypass);
		}
	}
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