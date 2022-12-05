library(tidyverse)

# Variables
max_ph <- 8
min_ph <- 6
ph_data <- read.csv("./analysis/data/ph_metrics.csv")

# Function to assign factor
assignState <- Vectorize(function(fl_injector, as_injector) {
  if (fl_injector == TRUE) {
    return(as.factor("fl-injector"))
  }
  
  if (as_injector == TRUE) {
    return(as.factor("as-injector"))
  }
  
  return(as.factor("none"))
})

# Add a iteration variable as index
ph_data$iteration <- 1:nrow(ph_data)

# Compute factor based on active actuator for each iteration
ph_data$actuator <- assignState(ph_data$fl.injector, ph_data$as.injector)

# Plot a subset of the ph_data
ph_data[1:200, ] %>% ggplot(aes(iteration, ph)) + 
  geom_line() + 
  geom_abline(aes(intercept = max_ph, slope = 0)) +
  geom_abline(aes(intercept = min_ph, slope = 0)) +
  geom_tile(aes(fill = actuator, height = Inf), alpha = 0.5)  +
  scale_fill_manual(values=c(NA, "blue3", "red3"), label=c("None", "Fl-injector", "As-injector"), na.value = NA) +
  labs(fill = "Active Actuators") +
  xlab("Iterations") + 
  ylab("pH Level") +
  theme_minimal() + 
  ggtitle("Sample of RARS System Controlling pH") + 
  theme(plot.title = element_text(hjust = 0.5))

# Save plot
ggsave(
  "ph_graph.jpeg",
  plot = last_plot(),
  device = "jpeg",
  path = "./analysis/graphs"
)

# Calculate percentage of iterations spent in the accepted ph interval
controlled_ph <- ph_data$ph >= min_ph & ph_data$ph <= max_ph

ph_percentage <- mean(controlled_ph) * 100