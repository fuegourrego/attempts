add_definitions(-DTRANSLATION_DOMAIN=\"kcmperformance\")


########### next target ###############

set(kcm_performance_PART_SRCS kcmperformance.cpp konqueror.cpp system.cpp )

ki18n_wrap_ui(kcm_performance_PART_SRCS konqueror_ui.ui system_ui.ui )

kcoreaddons_add_plugin(kcm_performance SOURCES ${kcm_performance_PART_SRCS} INSTALL_NAMESPACE "konqueror_kcms")
kcoreaddons_desktop_to_json(kcm_performance kcmperformance.desktop)

target_link_libraries(kcm_performance  KF5::ConfigWidgets KF5::KIOWidgets KF5::I18n Qt5::DBus )
