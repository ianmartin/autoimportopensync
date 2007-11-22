<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:include href="helper.xsl"/>

<xsl:strip-space elements="*" />
<xsl:output method="xml" indent="yes"/> 

<xsl:template match="note">
	<xsl:copy>
		<xsl:apply-templates />
    </xsl:copy>
</xsl:template>

<xsl:template match="Body">
    <Description>
		<Content>
		    <xsl:apply-templates/> 
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

<xsl:template match="Subject">
    <Summary>
		<Content>
		    <xsl:apply-templates/>
		</Content>
	</Summary>
</xsl:template>

<xsl:template match="Date">
    <Created>
		<Content>
		    <xsl:apply-templates/>
		</Content>
    </Created>
</xsl:template>

<!-- removing unknown elements -->
<xsl:template match="Color"/>
<xsl:template match="Height"/>
<xsl:template match="Left"/>
<xsl:template match="Top"/>
<xsl:template match="Width"/>


</xsl:stylesheet>
