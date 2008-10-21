<?xml version="1.0" ?>
<xsl:stylesheet version="1.0"
		xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
		xmlns:atom="http://www.w3.org/2005/Atom"
		xmlns:gd="http://schemas.google.com/g/2005">

<xsl:output method="xml" indent="yes"/>

<xsl:template match="child::entry">
  <event>
        <Comment>
	  <Content>
           <xsl:value-of select="descendant::atom:content"/>
	  </Content>
        </Comment>

        <Contact>
	  <Content>
           <xsl:value-of select="descendant::atom:author/atom:name"/>
	  </Content>
        </Contact>

	<Created>
	  <Content>
           <xsl:value-of select="descendant::atom:published"/>
	  </Content>
	</Created>

	<DateEnd>
	  <Content>
	    <xsl:choose>
	      <xsl:when test="descendant::gd:when/@endTime">
		<xsl:value-of select="descendant::gd:when/@endTime"/>
	      </xsl:when>
	      <xsl:otherwise>
		<xsl:variable name="recurrence">
		  <xsl:value-of select="descendant::gd:recurrence"/>
		</xsl:variable>

		<xsl:choose>
		  <xsl:when test="substring-after($recurrence,'DTEND;VALUE=DATE:')">
		    <xsl:value-of select="substring-before(substring-after($recurrence, 'DTEND;VALUE=DATE:'),'RRULE:')"/>
		  </xsl:when>
		  <xsl:otherwise>
		    <xsl:value-of select="substring-after(substring-before(substring-after($recurrence,'DTEND;TZID='),
					  'RRULE:'), ':')"/>
		  </xsl:otherwise>
		</xsl:choose>

	      </xsl:otherwise>
	    </xsl:choose>
	  </Content>
	</DateEnd>

	<DateStarted>
	  <Content>
	    <xsl:choose>
	      <xsl:when test="descendant::gd:when/@startTime">
		<xsl:value-of select="descendant::gd:when/@startTime"/>
	      </xsl:when>
	      <xsl:otherwise>
		<!-- TODO: remove trailing empty space... -->
		<xsl:variable name="recurrence">
		  <xsl:value-of select="descendant::gd:recurrence"/>
		</xsl:variable>

      		<xsl:choose>
		  <xsl:when test="substring-after($recurrence, 'DTSTART;VALUE=DATE:')">
		    <xsl:value-of select="substring-before(substring-after($recurrence, 'DTSTART;VALUE=DATE:'),'DTEND')"/>
		  </xsl:when>
		  <xsl:otherwise>
		    <xsl:value-of select="substring-after(substring-before(substring-after($recurrence,'DTSTART;TZID='),
					  'DTEND;'), ':')"/>
		  </xsl:otherwise>
      		</xsl:choose>

	      </xsl:otherwise>
	    </xsl:choose>
	  </Content>
	</DateStarted>

        <Description>
	  <Content>
           <xsl:value-of select="descendant::atom:title"/>
	  </Content>
        </Description>

	<LastModified>
	  <Content>
           <xsl:value-of select="descendant::atom:updated"/>
	  </Content>
	</LastModified>

	<Location>
	  <Content>
	    <xsl:value-of select="descendant::gd:where/@valueString"/>
	  </Content>
	</Location>

	<xsl:if test="descendant::gd:recurrence">
	  <RecurrenceRule>
	    <!-- TODO: remove trailing empty space... -->
	    <xsl:variable name="recurrence">
	      <xsl:value-of select="descendant::gd:recurrence"/>
	    </xsl:variable>

	    <Frequency>
	      <xsl:value-of select="substring-before(substring-after($recurrence,
				    'RRULE:FREQ='),';BYDAY=')"/>
	    </Frequency>
	    <Until>
	      <xsl:value-of select="substring-after($recurrence,';UNTIL=')"/>
	    </Until>
	    <ByDay>
	      <xsl:value-of select="substring-before(substring-after($recurrence,
				    ';BYDAY='),';')"/>
	    </ByDay>
	  </RecurrenceRule>
	</xsl:if>

	<Status>
	  <Content>
	    <xsl:variable name="status">
	      <xsl:value-of select="descendant::gd:eventStatus/@value"/>
	    </xsl:variable>
	    <xsl:choose>
	      <xsl:when test="substring-after($status, '#event.') =
			      'confirmed'">CONFIRMED</xsl:when>
	      <xsl:otherwise>CANCELLED</xsl:otherwise>
	    </xsl:choose>
	  </Content>
	</Status>

	<Uid>
	  <Content>
	    <xsl:value-of select="descendant::atom:id"/>
	  </Content>
	</Uid>

	<Url>
	  <Content>
	    <xsl:value-of select="descendant::atom:link[@rel='edit']/@href"/>
	  </Content>
	</Url>

  </event>
</xsl:template>


</xsl:stylesheet>
