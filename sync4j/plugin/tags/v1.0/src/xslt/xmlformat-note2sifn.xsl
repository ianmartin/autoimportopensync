<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:strip-space elements="*" />
<xsl:output method="xml" indent="yes"/>

<!-- removing unknown elements -->
<xsl:template match="Class"/>
<xsl:template match="LastModified"/>
<xsl:template match="XIrmcLuid"/>

<xsl:template match="note">
	<xsl:copy>
		<xsl:apply-templates />
    </xsl:copy>
</xsl:template>

<xsl:template match="Body/Content">
	<Body>
		<xsl:copy-of select="text()"/>
	</Body>
</xsl:template>

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

<xsl:template match="Created/Content">
	<Date>
		<xsl:copy-of select="text()"/>
	</Date>
</xsl:template>

<xsl:template match="Summary/Content">
	<Subject>
		<xsl:copy-of select="text()"/>
	</Subject>
</xsl:template>

</xsl:stylesheet>
