library(tidyverse)

# Variables
max_humidity <- 90
min_humidity <- 50
humidity_data <- read.csv("./analysis/data/humid_metrics.csv")

# Function to assign factor
assignState <- Vectorize(function(humidifier, dehumidifier) {
  if (humidifier == TRUE) {
    return(as.factor("humidifier"))
  }
  
  if (dehumidifier == TRUE) {
    return(as.factor("dehumidifier"))
  }
  
  return(as.factor("none"))
})

# Add a iteration variable as index
humidity_data$iteration <- 1:nrow(humidity_data)

# Compute factor based on active actuator for each iteration
humidity_data$actuator <- assignState(humidity_data$humidifier, humidity_data$de.humidifier)

# Plot a subset of the humidity_data
humidity_data[1:200, ] %>% ggplot(aes(iteration, humidity)) + 
  geom_line() + 
  geom_abline(aes(intercept = max_humidity, slope = 0)) +
  geom_abline(aes(intercept = min_humidity, slope = 0)) +
  geom_tile(aes(fill = actuator, height = Inf), alpha = 0.5)  +
  scale_fill_manual(values=c(NA, "blue3", "red3"), label=c("None", "Humidifier", "Dehumidifier"), na.value = NA) +
  labs(fill = "Active Actuators") +
  xlab("Iterations") + 
  ylab("Humidity Level") +
  theme_minimal() + 
  ggtitle("Sample of RARS System Controlling Humidity") + 
  theme(plot.title = element_text(hjust = 0.5))

# Save plot
ggsave(
  "humidity_graph.jpeg",
  plot = last_plot(),
  device = "jpeg",
  path = "./analysis/graphs"
)

# Calculate percentage of iterations spent in the accepted humidity interval
controlled_humidity <- humidity_data$humidity >= min_humidity & humidity_data$humidity <= max_humidity

humidity_percentage <- mean(controlled_humidity) * 100

