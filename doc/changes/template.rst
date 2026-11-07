{# Renders a release in the nested-bullet style used by doc/CHANGELOG.rst.
   User-facing entries are grouped by type; the "For developers" section is
   rendered as one more bullet, with its own types nested beneath it.
   towncrier already indents continuation lines by 2 (all_bullets), hence the
   indent() widths are 2 less than the text column. #}
- **{{ versiondata.version }} - {{ versiondata.date }}**

{% for section, _ in sections.items() %}
{% if section and sections[section] %}
  - {{ section }}

{% set pad = "    " %}
{% else %}
{% set pad = "  " %}
{% endif %}
{% for category, val in definitions.items() if category in sections[section] %}
{{ pad }}- {{ definitions[category]['name'] }}

{% for text, values in sections[section][category].items() %}
{{ pad }}  - {% if text %}{{ text|indent(pad|length + 2) }}{% if values %} ({{ values|join(', ') }}){% endif %}{% else %}{{ values|join(', ') }}{% endif %}


{% endfor %}
{% endfor %}
{% endfor %}
