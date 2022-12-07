library(tidyverse)

names <- c("Temperature", "Humidity", "pH")
values <- c(temperature_percentage, humidity_percentage, ph_percentage)
df <- data.frame(names, values)

df %>%
  ggplot(aes(names, values)) +
  geom_col(fill="blue") +
  ylab("Percentage of iterations under control") +
  xlab("Environmental Variable") +
  theme_minimal()

# Save plot
ggsave(
  "time_graph.jpeg",
  plot = last_plot(),
  device = "jpeg",
  path = "./analysis/graphs",
  width = 8,
  height = 10,
  units = "cm"
)