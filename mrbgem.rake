MRuby::Gem::Specification.new('mruby-json') do |spec|
  spec.license = 'MIT'
  spec.authors = 'mattn'
  spec.cc.defines << 'JSON_FIXED_NUMBER'
  spec.add_dependency 'mruby-metaprog', :core => 'mruby-metaprog' if Dir.exist? "#{MRUBY_ROOT}/mrbgems/mruby-metaprog"
end
