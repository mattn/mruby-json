MRuby::Gem::Specification.new('mruby-json') do |spec|
  spec.license = 'MIT'
  spec.authors = 'mattn'

  spec.cc.defines << 'strtod=my_strtod' << '__strtod=my_strtod'
end
