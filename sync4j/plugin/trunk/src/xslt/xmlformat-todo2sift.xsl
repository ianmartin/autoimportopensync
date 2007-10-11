<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:include href="helper.xsl"/>

<xsl:strip-space elements="*" />
<xsl:output method="xml" indent="yes"/>

<xsl:template match="todo">
	<task>
		<!-- don't know if in the following conditionals only the existence of
			an element should be checked or also for a existing/valid content ... -->
		<xsl:choose>
			<xsl:when test="Alarm">
				<xsl:apply-templates select="Alarm"/>
			</xsl:when>
			<xsl:otherwise>
				<ReminderSet>0</ReminderSet>
			</xsl:otherwise>
		</xsl:choose>
		
		<xsl:apply-templates select="Attach"/>
		<xsl:apply-templates select="Attendee"/>
		<xsl:apply-templates select="CalendarScale"/>
		<xsl:apply-templates select="Categories"/>
		<xsl:apply-templates select="Class"/>
		<xsl:apply-templates select="Comment"/>

		<xsl:choose>
			<xsl:when test="Completed">
				<xsl:apply-templates select="Completed"/>
			</xsl:when>
			<xsl:otherwise>
				<Complete>0</Complete>
			</xsl:otherwise>
		</xsl:choose>
		
		<xsl:apply-templates select="Contact"/>
		<xsl:apply-templates select="Created"/>
		<xsl:apply-templates select="DateCalendarCreated"/>
		<xsl:apply-templates select="DateStarted"/>
		<xsl:apply-templates select="Description"/>
		<xsl:apply-templates select="Due"/>
		<xsl:apply-templates select="Duration"/>
		<xsl:apply-templates select="ExceptionDateTime"/>
		<xsl:apply-templates select="ExceptionRule"/>
		<xsl:apply-templates select="Geo"/>
		<xsl:apply-templates select="LastModified"/>
		<xsl:apply-templates select="Location"/>
		<xsl:apply-templates select="Method"/>
		<xsl:apply-templates select="Organizer"/>
		<xsl:apply-templates select="PercentComplete"/>
		<xsl:apply-templates select="Priority"/>
		<xsl:apply-templates select="ProductID"/>
		<xsl:apply-templates select="RecurrenceDateTime"/>
		<xsl:apply-templates select="RecurrenceId"/>
		<xsl:apply-templates select="Related"/>
		<xsl:apply-templates select="Resources"/>

		<xsl:choose>
			<xsl:when test="RecurrenceRule">
				<xsl:apply-templates select="RecurrenceRule"/>
			</xsl:when>
			<xsl:otherwise>
				<IsRecurring>0</IsRecurring>
			</xsl:otherwise>
		</xsl:choose>
		
		<xsl:apply-templates select="RStatus"/>
		<xsl:apply-templates select="Sequence"/>
		<xsl:apply-templates select="Status"/>
		<xsl:apply-templates select="Summary"/>
		<xsl:apply-templates select="Uid"/>
		<xsl:apply-templates select="Url"/>
		<xsl:apply-templates select="Version"/>
	</task>
</xsl:template>

<xsl:template match="Alarm">
	<ReminderSet>1</ReminderSet>
	<ReminderTime>
		<xsl:copy-of select="AlarmTrigger/text()"/>
	</ReminderTime>
</xsl:template>

<xsl:template match="Attach"/>

<xsl:template match="Attendee"/>

<xsl:template match="CalendarScale"/>

<!-- flattening 'Categorie' tags into one 'Categories' tag; values are separated
	by a ';' -->
<xsl:template match="Categories">
	<Categories>
		<xsl:for-each select="Category">
			<xsl:value-of select="text()"/>
			<xsl:if test="position()!=last()">
				<xsl:text>;</xsl:text>
			</xsl:if> 
		</xsl:for-each>
		<xsl:text/>
	</Categories>
</xsl:template>

<xsl:template match="Class">
	<Sensitivity>
		<xsl:choose>
			<xsl:when test="Content/text()='PUBLIC'">
				<xsl:text>0</xsl:text>
			</xsl:when>
			<xsl:when test="Content/text()='CONFIDENTIAL'">
				<xsl:text>3</xsl:text>
			</xsl:when>
			<xsl:when test="Content/text()='PRIVATE'">
				<xsl:text>2</xsl:text>
			</xsl:when>
		</xsl:choose>
	</Sensitivity>
</xsl:template>

<xsl:template match="Comment"/>

<xsl:template match="Completed">
	<Complete>1</Complete>
	<DateCompleted>
		<xsl:copy-of select="Content/text()"/>
	</DateCompleted>
</xsl:template>

<xsl:template match="Contact"/>

<xsl:template match="Created"/>

<xsl:template match="DateCalendarCreated"/>

<xsl:template match="DateStarted">
	<StartDate>
		<xsl:call-template name="ISO8601toSync4j">
			<xsl:with-param name="string" select="Content/text()"/>
		</xsl:call-template>
	</StartDate>
</xsl:template>

<xsl:template match="Description">
	<Body>
		<xsl:copy-of select="Content/text()"/>
	</Body>
</xsl:template>

<xsl:template match="Due">
	<DueDate>
		<xsl:call-template name="ISO8601toSync4j">
			<xsl:with-param name="string" select="Content/text()"/>
		</xsl:call-template>
	</DueDate>
</xsl:template>

<xsl:template match="Duration"/>

<xsl:template match="ExceptionDateTime"/>

<xsl:template match="ExceptionRule"/>

<xsl:template match="Geo"/>

<xsl:template match="LastModified"/>

<xsl:template match="Location"/>

<xsl:template match="Method"/>

<xsl:template match="Organizer"/>

<xsl:template match="PercentComplete">
	<PercentComplete>
		<xsl:copy-of select="Content/text()"/>
	</PercentComplete>
</xsl:template>

<xsl:template match="Priority">
	<Importance>
		<xsl:choose>
			<xsl:when test="Content/text()&lt;'10' and Content/text()&gt;'6'">
				<xsl:text>0</xsl:text>
			</xsl:when>
			<xsl:when test="Content/text()&lt;'7' and Content/text()&gt;'3'">
				<xsl:text>1</xsl:text>
			</xsl:when>
			<xsl:when test="Content/text()&lt;'4' and Content/text()&gt;'0'">
				<xsl:text>2</xsl:text>
			</xsl:when>
			<xsl:otherwise> <!-- e.g. if undefined(0) it is set to low -->
				<xsl:text>0</xsl:text>
			</xsl:otherwise>
		</xsl:choose>
	</Importance>
</xsl:template>

<xsl:template match="ProductID"/>

<xsl:template match="RecurrenceDateTime"/>

<xsl:template match="RecurrenceId"/>

<xsl:template match="Related"/>

<xsl:template match="Resources"/>

<xsl:template match="RecurrenceRule">
	<IsRecurring>1</IsRecurring>
	<RecurrenceType>
		<xsl:choose>
			<xsl:when test="Frequency/text()='DAILY'">
				<xsl:text>0</xsl:text>
			</xsl:when>
			<xsl:when test="Frequency/text()='WEEKLY'">
				<xsl:text>1</xsl:text>
			</xsl:when>
			<xsl:when test="Frequency/text()='MONTHLY'">
				<xsl:text>2</xsl:text>
			</xsl:when>
			<xsl:when test="Frequency/text()='YEARLY'">
				<xsl:text>5</xsl:text>
			</xsl:when>
		</xsl:choose>
	</RecurrenceType>
	<PatternEndDate>
		<xsl:copy-of select="Until/text()"/>
	</PatternEndDate>
	<Occurrences>
		<xsl:copy-of select="Count/text()"/>
	</Occurrences>
	<xsl:copy-of select="Interval"/>
</xsl:template>

<xsl:template match="RStatus"/>

<xsl:template match="Sequence"/>

<xsl:template match="Status">
	<Status>
		<xsl:choose>
			<xsl:when test="Content/text()='COMPLETED'">
				<xsl:text>2</xsl:text>
			</xsl:when>
			<xsl:otherwise>
				<xsl:text>1</xsl:text>
			</xsl:otherwise>
		</xsl:choose>
	</Status>
</xsl:template>

<xsl:template match="Summary">
	<Subject>
		<xsl:copy-of select="Content/text()"/>
	</Subject>
</xsl:template>

<xsl:template match="Uid"/>

<xsl:template match="Url"/>

<xsl:template match="Version"/>


</xsl:stylesheet>