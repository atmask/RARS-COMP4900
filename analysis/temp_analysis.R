library(tidyverse)

# Variables
max_temp <- 25
min_temp <- 20
temp_data <- read.csv("./analysis/data/temp_metrics.csv")

# Function to assign factor
assignState <- Vectorize(function(heater, cooling) {
  if (heater == TRUE) {
    return(as.factor("heater"))
  }
  
  if (cooling == TRUE) {
    return(as.factor("cooling"))
  }
  
  return(as.factor("none"))
})

# Merge heater and a.c into actuator variable
temp_data$actuator <- assignState(temp_data$heater, temp_data$a.c)

# Add a iteration variable as index
temp_data$iteration <- 1:nrow(temp_data)

# Plot a subset of the temp_data
temp_data[1:200, ] %>% ggplot(aes(iteration, temperature)) + 
  geom_line() + 
  geom_abline(aes(intercept = max_temp, slope = 0)) +
  geom_abline(aes(intercept = min_temp, slope = 0)) +
  geom_tile(aes(fill = actuator, height = Inf), alpha = 0.5)  +
  scale_fill_manual(values=c(NA, "blue3", "red3"), labels = c("None", "Cooling", "Heater"), na.value = NA) +
  labs(fill = "Active Actuators") +
  xlab("Iterations") + 
  ylab("Temperature (°C)") +
  theme_minimal() + 
  ggtitle("Sample of RARS System Controlling Temperature") + 
  theme(plot.title = element_text(hjust = 0.5))

# Save plot
ggsave(
  "temp_graph.jpeg",
  plot = last_plot(),
  device = "jpeg",
  path = "./analysis/graphs"
)

# Calculate percentage of iterations spent in the accepted temperature interval
good_temperature <- temp_data$temperature >= min_temp & temp_data$temperature <= max_temp

temperature_percentage <- mean(good_temperature) * 100



