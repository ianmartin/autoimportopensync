<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:include href="helper.xsl"/>

<xsl:strip-space elements="*" />
<xsl:output method="xml" indent="yes"/> 

<xsl:template match="task">
    <todo>
    	<xsl:apply-templates select="ActualWork"/>
    	<xsl:apply-templates select="BillingInformation"/>
    	<xsl:apply-templates select="Body"/>
    	<xsl:apply-templates select="Categories"/>
    	<xsl:apply-templates select="Companies"/>
    	<xsl:apply-templates select="Complete"/>
    	<xsl:apply-templates select="DueDate"/>
    	<xsl:apply-templates select="DateCompleted"/>
    	<xsl:apply-templates select="Importance"/>
    	<xsl:apply-templates select="Mileage"/>
    	<xsl:apply-templates select="PercentComplete"/>
    	<xsl:apply-templates select="ReminderSet"/>
    	<xsl:apply-templates select="ReminderSoundFile"/>
    	<xsl:apply-templates select="ReminderOptions"/>
    	<xsl:apply-templates select="IsRecurring"/>
    	<xsl:apply-templates select="Sensitivity"/>
    	<xsl:apply-templates select="StartDate"/>
    	<xsl:apply-templates select="Status"/>
    	<xsl:apply-templates select="Subject"/>
    	<xsl:apply-templates select="TeamTask"/>
    	<xsl:apply-templates select="TotalWork"/>

    	<xsl:if test="ReminderSet/text()=1">
    		<!-- the attribute Value defines the trigger type, which can be
    			either DATE-TIME(1) or DURATION(2). Since the only trigger we have
    			is ReminderTime, Value is set to 1 -->
			<Alarm Value="DATE-TIME">
				<AlarmTrigger>
					<xsl:apply-templates select="ReminderTime"/>
				</AlarmTrigger>
			</Alarm>
    	</xsl:if>

		<xsl:if test="IsRecurring/text()=1">
    		<RecurrenceRule>
				<xsl:apply-templates select="RecurrenceType"/>
				<xsl:apply-templates select="DayOfMonth"/>
				<xsl:apply-templates select="DayOfWeekMask"/>
				<xsl:apply-templates select="Interval"/>
				<xsl:apply-templates select="MonthOfYear"/>
				<xsl:apply-templates select="NoEndDate"/>
				<xsl:apply-templates select="Occurrences"/>
				<xsl:apply-templates select="PatternStartDate"/>
				<xsl:apply-templates select="PatternEndDate"/>
			</RecurrenceRule>
		</xsl:if> 
    </todo>
</xsl:template>

<xsl:template match="ActualWork"/>

<xsl:template match="BillingInformation"/>

<xsl:template match="Body">
	<Description>
		<Content>
			<xsl:copy-of select="text()"/>
		</Content>
	</Description>
</xsl:template>

<xsl:template match="Categories">
	<Categories>
		<xsl:call-template name="tokenize">
			<xsl:with-param name="string" select="text()" />
			<xsl:with-param name="tag" select="'Category'" />
			<!-- default delimiter is ';' -->
		</xsl:call-template>
	</Categories>
</xsl:template>

<xsl:template match="Companies"/>

<xsl:template match="Complete"/>

<xsl:template match="DueDate">
	<Due>
		<Content>
			<xsl:call-template name="sync4jtoISO8601">
				<xsl:with-param name="string" select="text()"/>
			</xsl:call-template>
		</Content>
	</Due>
</xsl:template>

<xsl:template match="DateCompleted">
	<Completed>
		<Content>
			<xsl:call-template name="sync4jtoISO8601">
				<xsl:with-param name="string" select="text()"/>
			</xsl:call-template>
		</Content>
	</Completed>
</xsl:template>

<xsl:template match="Importance">
	<Priority>
		<Content>
			<xsl:choose>
				<xsl:when test="text()='0'"> <!-- low priority -->
					<xsl:text>9</xsl:text>
				</xsl:when>
				<xsl:when test="text()='1'"> <!-- normal priority -->
					<xsl:text>5</xsl:text>
				</xsl:when>
				<xsl:when test="text()='2'"> <!-- high priority -->
					<xsl:text>1</xsl:text> <!-- according to vcalendar spec 0 means undefined -->
				</xsl:when>
			</xsl:choose>
		</Content>
	</Priority>
</xsl:template>

<xsl:template match="Mileage"/>

<xsl:template match="PercentComplete">
	<PercentComplete>
		<Content>
			<xsl:copy-of select="text()"/>
		</Content>
	</PercentComplete>
</xsl:template>

<xsl:template match="ReminderSet"/>

<xsl:template match="ReminderTime">
	<xsl:copy-of select="text()"/>
</xsl:template>

<xsl:template match="ReminderSoundFile"/>

<xsl:template match="ReminderOptions"/>

<xsl:template match="IsRecurring"/>

<xsl:template match="Sensitivity">
	<Class>
		<Content>
			<xsl:choose>
				<xsl:when test="text()='0'"> <!-- olNormal -> PUBLIC -->
					<xsl:text>PUBLIC</xsl:text>
				</xsl:when>
				<xsl:when test="text()='3'"> <!-- olConfidential -> CONFIDENTIAL -->
					<xsl:text>CONFIDENTIAL</xsl:text>
				</xsl:when>
				<xsl:otherwise> <!-- all other values default do PRIVATE -->
					<xsl:text>PRIVATE</xsl:text>
				</xsl:otherwise>
			</xsl:choose>
		</Content>
	</Class>
</xsl:template>

<xsl:template match="StartDate">
	<DateStarted>
		<Content>
			<xsl:call-template name="sync4jtoISO8601">
				<xsl:with-param name="string" select="text()"/>
			</xsl:call-template>
		</Content>
	</DateStarted>
</xsl:template>

<xsl:template match="Status">
	<Status>
		<Content>
			<xsl:choose>
				<xsl:when test="text()='2'"> <!-- complete -->
					<xsl:text>COMPLETED</xsl:text>
				</xsl:when>
				<xsl:otherwise>
					<xsl:text>IN-PROCESS</xsl:text>
 					<!-- default state set to 'in process' since there is no
						other match	in the meaning of the stati:
						opensync: NEEDS-ACTION, COMPLETED, IN-PROCESS, CANCELLED.
						sync4j: olTaskNotStarted, olTaskInProgress,	olTaskCompleted,
						olTaskWaiting, olTaskWaiting -->
				</xsl:otherwise>
			</xsl:choose>
		</Content>
	</Status>
</xsl:template>

<xsl:template match="Subject">
	<Summary>
		<Content>
			<xsl:copy-of select="text()"/>
		</Content>
	</Summary>
</xsl:template>

<xsl:template match="TeamTask"/>

<xsl:template match="TotalWork"/>


<xsl:template match="DayOfMonth"/>

<xsl:template match="DayOfWeekMask"/>

<xsl:template match="Interval">
	<xsl:copy-of select="."/>
</xsl:template>

<xsl:template match="Instance"/>

<xsl:template match="MonthOfYear"/>

<xsl:template match="NoEndDate"/>

<xsl:template match="Occurrences">
	<Count>
		<xsl:copy-of select="text()"/>
	</Count>
</xsl:template>

<xsl:template match="PatternStartDate"/>

<xsl:template match="PatternEndDate">
	<Until>
		<xsl:copy-of select="text()"/>
	</Until>
</xsl:template>

<xsl:template match="RecurrenceType">
	<Frequency>
		<xsl:choose>
			<xsl:when test="text()='0'">
				<xsl:text>DAILY</xsl:text>
			</xsl:when>
			<xsl:when test="text()='1'">
				<xsl:text>WEEKLY</xsl:text>
			</xsl:when>
			<xsl:when test="text()='2'">
				<xsl:text>MONTHLY</xsl:text>
			</xsl:when>
			<xsl:when test="text()='5'">
				<xsl:text>YEARLY</xsl:text>
			</xsl:when>
			<xsl:otherwise>
				<!-- default set to daily... -->
				<xsl:text>1</xsl:text>
			</xsl:otherwise>
		</xsl:choose>
	</Frequency>
</xsl:template>

</xsl:stylesheet>