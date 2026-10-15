#include "core/plugins/plugin_pool.h"

#include "core/log.h"
#include "engine/mix_renderer.h"
#include "engine/plugins/builtin_plugins.h"
#include "engine/plugins/hosted_plugin.h"

#include <cmath>

PluginPool::PluginPool(PluginLibrary& library) : library(library)
{
}

void PluginPool::use_rate(double rate)
{
	if (std::lround(rate) == std::lround(sample_rate))
		return;

	while (!instances.empty())
		retire(instances.begin()->first);

	chains.clear();
	sample_rate = rate;
}

void PluginPool::forget_removed(const Project& project)
{
	std::erase_if(chains, [&](const auto& entry) { return project.find_track(entry.first) == nullptr; });

	std::vector<PluginId> removed;

	for (const auto& [id, instance] : instances)
		if (project.find_plugin(id) == nullptr)
			removed.push_back(id);

	for (const auto id : removed)
		retire(id);
}

std::shared_ptr<PluginChain> PluginPool::chain_for(const Track& track)
{
	std::vector<PluginChain::Slot> slots;

	for (const auto& plugin : track.plugins)
		if (auto instance = instance_for(plugin))
			slots.push_back({std::move(instance), plugin.bypass});

	auto& chain = chains[track.id];

	if (slots.empty())
		chain = nullptr;
	else if (chain == nullptr || chain->slots() != slots)
		chain = std::make_shared<PluginChain>(std::move(slots));

	return chain;
}

std::shared_ptr<PluginProcessor> PluginPool::find_instance(PluginId id) const
{
	const auto found = instances.find(id);
	return found == instances.end() ? nullptr : found->second.processor;
}

bool PluginPool::is_missing(PluginId id) const
{
	const auto found = instances.find(id);
	return found != instances.end() && found->second.processor == nullptr;
}

void PluginPool::retry_missing()
{
	if (std::erase_if(instances, [](const auto& entry) { return entry.second.processor == nullptr; }) > 0)
		++availability_revision;
}

bool PluginPool::take_changes()
{
	auto changed = false;

	for (auto& [id, instance] : instances) {
		if (instance.processor == nullptr || instance.processor->changes() == instance.seen_changes)
			continue;

		instance.seen_changes = instance.processor->changes();
		changed = true;
	}

	return changed;
}

void PluginPool::store_states(Project& project) const
{
	for (auto& track : project.tracks)
		for (auto& plugin : track.plugins) {
			if (const auto processor = find_instance(plugin.id))
				plugin.state = processor->get_state();
			else if (const auto retired = retired_states.find(plugin.id); retired != retired_states.end())
				plugin.state = retired->second;
		}
}

void PluginPool::clear()
{
	chains.clear();
	instances.clear();
	retired_states.clear();
}

std::shared_ptr<PluginProcessor> PluginPool::instance_for(const Plugin& plugin)
{
	if (sample_rate <= 0.0)
		return nullptr;

	if (const auto found = instances.find(plugin.id); found != instances.end())
		return found->second.processor;

	auto processor = create_instance(plugin);
	instances[plugin.id] = {processor, processor != nullptr ? processor->changes() : 0};
	return processor;
}

std::shared_ptr<PluginProcessor> PluginPool::create_instance(const Plugin& plugin)
{
	auto instance = plugin.format == BUILTIN_FORMAT ? create_builtin_plugin(plugin.identifier) : create_hosted(plugin);

	if (instance == nullptr && plugin.format == BUILTIN_FORMAT)
		log_plugin_problem(plugin, "is not a built-in effect of this version");

	if (instance == nullptr)
		return nullptr;

	const auto retired = retired_states.extract(plugin.id);
	instance->set_state(retired.empty() ? plugin.state : retired.mapped());
	instance->prepare({sample_rate, MixRenderer::CHUNK_SAMPLES});
	return instance;
}

std::shared_ptr<PluginProcessor> PluginPool::create_hosted(const Plugin& plugin)
{
	const auto description = library.find(plugin);

	if (description == nullptr) {
		log_plugin_problem(plugin, "is not installed");
		return nullptr;
	}

	juce::String error;
	auto loaded = library.formats.createPluginInstance(*description, sample_rate, MixRenderer::CHUNK_SAMPLES, error);

	if (loaded == nullptr) {
		log_plugin_problem(plugin, "could not be loaded: " + error);
		return nullptr;
	}

	auto hosted = std::make_shared<HostedPlugin>(std::move(loaded));

	if (!hosted->has_usable_layout()) {
		log_plugin_problem(plugin, "needs more than two channels");
		return nullptr;
	}

	if (description->version != plugin.version)
		log_event("Plugin " + plugin.name + " was saved with version " + plugin.version + ", loading version " + description->version);

	return hosted;
}

void PluginPool::log_plugin_problem(const Plugin& plugin, const juce::String& problem) const
{
	log_event("Plugin " + plugin.name + " (" + plugin.format + " " + plugin.identifier + ") " + problem + "; its track plays without it");
}

void PluginPool::retire(PluginId id)
{
	const auto found = instances.find(id);

	if (found == instances.end())
		return;

	if (found->second.processor != nullptr)
		retired_states[id] = found->second.processor->get_state();

	instances.erase(found);
}
