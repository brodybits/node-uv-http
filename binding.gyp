{
  "targets": [
    {
      "target_name": "uvhttp",
      "sources": [
        "binding.cc"
      ],
      "include_dirs": [
        "<!(node -e \"require ('nan')\")",
        "."
      ]
    }
  ]
}