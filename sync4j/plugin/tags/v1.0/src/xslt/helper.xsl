<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:template match="node()|@*" mode="tokenize" name="tokenize">
	<xsl:param name="string" select="string()" />
	<xsl:param name="delim" select="'&#x3B;'" />
	<xsl:param name="tag" select="'tag'" />
	<xsl:variable name="multitoken" select="contains($string, $delim)" />
	<xsl:variable name="token">
		<xsl:choose>
			<xsl:when test="$multitoken">
				<xsl:value-of select="normalize-space(substring-before($string, $delim))"/>
			</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="normalize-space($string)" />
			</xsl:otherwise>
		</xsl:choose>
	</xsl:variable>
	<xsl:if test="string($token)">
		<xsl:element name="{$tag}" >
			<xsl:value-of select="$token" />
		</xsl:element>
	</xsl:if>
	<xsl:if test="$multitoken">
		<xsl:call-template name="tokenize">
			<xsl:with-param name="string" select="substring-after($string, $delim)" />
			<xsl:with-param name="delim" select="$delim" />
			<xsl:with-param name="tag" select="$tag" />
		</xsl:call-template>
	</xsl:if>
</xsl:template>

<xsl:template name="globalReplace">
	<xsl:param name="outputString"/>
	<xsl:param name="target"/>
	<xsl:param name="replacement"/>
	<xsl:choose>
		<xsl:when test="contains($outputString,$target)">
			<xsl:value-of select="concat(substring-before($outputString,$target),$replacement)"/>
			<xsl:call-template name="globalReplace">
				<xsl:with-param name="outputString" select="substring-after($outputString,$target)"/>
				<xsl:with-param name="target" select="$target"/>
				<xsl:with-param name="replacement" select="$replacement"/>
			</xsl:call-template>
		</xsl:when>
		<xsl:otherwise>
			<!-- TODO I'm concatenating here, because I don't know how to add T000000Z
				in the calling template. This has to be fixed -->
			<xsl:value-of select="concat($outputString,'T000000Z')"/>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template name="sync4jtoISO8601">
	<xsl:param name="string"/>
	<xsl:choose>
		<xsl:when test="contains($string,'-')">
			<xsl:call-template name="globalReplace">
				<xsl:with-param name="outputString" select="$string"/>
				<xsl:with-param name="target" select="'-'"/>
				<xsl:with-param name="replacement" select="''"/>
			</xsl:call-template>
		</xsl:when>
		<xsl:otherwise>
			<xsl:copy-of select="$string"/>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template name="ISO8601toSync4j">
	<xsl:param name="string"/>
	<xsl:value-of select="concat(substring($string,1,4),'-',substring($string,5,2),
		'-',substring($string,7,2))"/>
</xsl:template>

</xsl:stylesheet>