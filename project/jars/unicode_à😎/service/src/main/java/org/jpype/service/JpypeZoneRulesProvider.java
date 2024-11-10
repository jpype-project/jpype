package org.jpype.service;

import java.time.ZoneId;
import java.time.zone.ZoneRules;
import java.time.zone.ZoneRulesProvider;
import java.util.Collections;
import java.util.NavigableMap;
import java.util.Set;
import java.util.TreeMap;

public class JpypeZoneRulesProvider extends ZoneRulesProvider {

	@Override
	protected Set<String> provideZoneIds() {
		return Collections.singleton("JpypeTest/Timezone");
	}

	@Override
	protected ZoneRules provideRules(String zoneId, boolean forCaching) {
		return ZoneId.of("UTC").getRules();
	}

	@Override
	protected NavigableMap<String, ZoneRules> provideVersions(String zoneId) {
		return new TreeMap<>();
	}

}
