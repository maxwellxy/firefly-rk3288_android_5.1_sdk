<?xml version="1.0"?>
<xsl:stylesheet
   version="1.1"
   xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:output method="html" indent="no" encoding="ISO-8859-1" />

<xsl:template match="toc">

   <html>
      <head />
      <body>
         <h1><xsl:value-of select="@label" /></h1>
         <ul>

            <xsl:apply-templates />
         </ul>
      </body>
   </html>
</xsl:template>

<xsl:template match="topic">

   <li>
      <xsl:choose>
         <xsl:when test="@href">
            <!-- Only add a hyperlink when there is something to link to -->
            <xsl:element name="a">
               <xsl:attribute name="href">

                  <xsl:value-of select="@href" />
                  </xsl:attribute>
               <xsl:value-of select="@label" />
            </xsl:element>
         </xsl:when>
         <xsl:otherwise>

            <xsl:value-of select="@label" />
         </xsl:otherwise>
      </xsl:choose>

      <!-- If there are any nested topics, then start a new sub-list -->
      <xsl:if test="descendant::topic">
         <ul>

            <xsl:apply-templates/>
         </ul>
      </xsl:if>
   </li>
</xsl:template>

</xsl:stylesheet>
