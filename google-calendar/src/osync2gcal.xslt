<?xml version="1.0" ?>
<xsl:stylesheet version="1.0"
		xmlns="http://www.w3.org/2005/Atom"
		xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
		xmlns:gd="http://schemas.google.com/g/2005">

<xsl:output method="xml" indent="yes"/>

<xsl:template match="child::event">
<entry>

	<id>
	    <xsl:value-of select="descendant::Uid/Content"/>
	</id>

	<published>
	    <xsl:value-of select="descendant::Created/Content"/>
	</published>

	<updated>
	    <xsl:value-of select="descendant::LastModified/Content"/>
	</updated>

	<category>
	  <xsl:attribute name="scheme">http://schemas.google.com/g/2005#kind</xsl:attribute>
	  <xsl:attribute name="term">http://schemas.google.com/contact/2008#event</xsl:attribute>
	</category>

	<title>
	  <xsl:attribute name="type">text</xsl:attribute>
	    <xsl:value-of select="descendant::Description/Content"/>
	</title>

	<content>
	  <xsl:attribute name="type">text</xsl:attribute>
	    <xsl:value-of select="descendant::Comment/Content"/>
	</content>

	<link>
	  <xsl:attribute name="rel">edit</xsl:attribute>
	  <xsl:attribute name="type">application/atom+xml</xsl:attribute>
	  <xsl:attribute name="href">
	    <xsl:value-of select="descendant::Url/Content"/>
	  </xsl:attribute>
	</link>

	<author>
	  <name><xsl:value-of select="descendant::Contact/Content"/></name>
	  <!-- TODO: add author's email -->
	</author>

	<!-- Hard-coded for while... -->
	<gd:visibility value="http://schemas.google.com/g/2005#event.default"/>
	<gd:transparency value="http://schemas.google.com/g/2005#event.opaque"/>

	<gd:eventStatus>
	  <xsl:attribute name="value">
	    <xsl:choose>
	      <xsl:when test="descendant::Status/Content = 'CONFIRMED'">http://schemas.google.com/g/2005#event.confirmed</xsl:when>
	      <xsl:otherwise>http://schemas.google.com/g/2005#event.cancelled</xsl:otherwise>
	    </xsl:choose>
	  </xsl:attribute>
	</gd:eventStatus>

	<gd:when>
	  <xsl:attribute name="startTime"><xsl:value-of select="descendant::DateStarted/Content"/></xsl:attribute>
	  <xsl:attribute name="endTime"><xsl:value-of select="descendant::DateEnd/Content"/></xsl:attribute>
	</gd:when>

	<gd:where>
	  <xsl:attribute name="valueString"><xsl:value-of select="descendant::Location/Content"/></xsl:attribute>
	</gd:where>


</entry>

</xsl:template>


</xsl:stylesheet>
