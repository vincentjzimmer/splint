/*
** Splint - annotation-assisted static program checker
** Copyright (C) 1994-2002 University of Virginia,
**         Massachusetts Institute of Technology
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of the GNU General Public License as published by the
** Free Software Foundation; either version 2 of the License, or (at your
** option) any later version.
** 
** This program is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** General Public License for more details.
** 
** The GNU General Public License is available from http://www.gnu.org/ or
** the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
** MA 02111-1307, USA.
**
** For information on splint: info@splint.org
** To report a bug: splint-bug@splint.org
** For more information: http://www.splint.org
*/

/*
** constraint.c
*/

/* #define DEBUGPRINT 1 */

# include <ctype.h> /* for isdigit */
# include "splintMacros.nf"
# include "basic.h"
# include "cgrammar.h"
# include "cgrammar_tokens.h"

# include "exprChecks.h"
# include "exprNodeSList.h"

/*@i33*/

/*@access exprNode @*/


static /*@only@*/ cstring  constraint_printDetailedPostCondition (/*@observer@*/ /*@temp@*/ constraint p_c);


static /*@notnull@*/ /*@special@*/ constraint constraint_makeNew (void)
     /*@post:isnull result->or, result->orig,  result->generatingExpr, result->fcnPre @*/
     /*@defines result->or, result->generatingExpr, result->orig, result->fcnPre @*/;
     
static void
advanceField (char **s)
{
  reader_checkChar (s, '@');
}

# if 0
static constraint makeConstraintParse2 (constraintExpr l, lltok relOp, exprNode cconstant)    
{
  char *t;
  int c;
  constraint ret;
  ret = constraint_makeNew ();
  llassert (constraintExpr_isDefined (l));
      
  ret->lexpr = constraintExpr_copy (l);


  if (relOp.tok == GE_OP)
      ret->ar = GTE;
  else if (relOp.tok == LE_OP)
    ret->ar = LTE;
  else if (relOp.tok == EQ_OP)
    ret->ar = EQ;
  else
  llfatalbug (message ("Unsupported relational operator"));


  t =  cstring_toCharsSafe (exprNode_unparse (cconstant));
  c = atoi ( t);
  ret->expr = constraintExpr_makeIntLiteral (c);

  ret->post = TRUE;
  DPRINTF (("GENERATED CONSTRAINT:"));
  DPRINTF ((message ("%s", constraint_print (ret))));
  return ret;
}
# endif

bool constraint_same (constraint c1, constraint c2)
{
  llassert (c1 != NULL);
  llassert (c2 != NULL);

  if (c1->ar != c2->ar)
    {
      return FALSE;
    }
  
  if (!constraintExpr_similar (c1->lexpr, c2->lexpr))
    {
      return FALSE;
    }

  if (!constraintExpr_similar (c1->expr, c2->expr))
    {
      return FALSE;
    }

  return TRUE;
}

constraint makeConstraintParse3 (constraintExpr l, lltok relOp, constraintExpr r)     
{
  constraint ret;
  ret = constraint_makeNew ();
  llassert (constraintExpr_isDefined (l));
    
  ret->lexpr = constraintExpr_copy (l);

  if (relOp.tok == GE_OP)
      ret->ar = GTE;
  else if (relOp.tok == LE_OP)
    ret->ar = LTE;
  else if (relOp.tok == EQ_OP)
    ret->ar = EQ;
  else
  llfatalbug ( message ("Unsupported relational operator"));

  ret->expr = constraintExpr_copy (r);

  ret->post = TRUE;

  ret->orig = constraint_copy (ret);

  ret = constraint_simplify (ret);
  /* ret->orig = ret; */

  DPRINTF (("GENERATED CONSTRAINT:"));
  DPRINTF ((message ("%s", constraint_print (ret))));
  return ret;
}

constraint constraint_copy (/*@temp@*/ /*@observer@*/ constraint c)
{
  constraint ret;

  llassert (constraint_isDefined (c));

  ret = constraint_makeNew ();
  ret->lexpr = constraintExpr_copy (c->lexpr);
  ret->ar = c->ar;
  ret->expr =  constraintExpr_copy (c->expr);
  ret->post = c->post;
  /*@-assignexpose@*/
  ret->generatingExpr = c->generatingExpr;
  /*@=assignexpose@*/
  
  /*@i33 fix this*/
  if (c->orig != NULL)
    ret->orig = constraint_copy (c->orig);
  else
    ret->orig = NULL;

  if (c->or != NULL)
    ret->or = constraint_copy (c->or);
  else
    ret->or = NULL;

  ret->fcnPre = c->fcnPre;
  
  return ret;
}

/*like copy expect it doesn't allocate memory for the constraint*/

void constraint_overWrite (constraint c1, constraint c2) 
{
  llassert (constraint_isDefined (c1));

  llassert (c1 != c2);

  DPRINTF ((message ("OverWriteing constraint %q with %q", constraint_print (c1),
		   constraint_print (c2))));
  
  constraintExpr_free (c1->lexpr);
  constraintExpr_free (c1->expr);
  
  c1->lexpr = constraintExpr_copy (c2->lexpr);
  c1->ar = c2->ar;
  c1->expr =  constraintExpr_copy (c2->expr);
  c1->post = c2->post;

  if (c1->orig != NULL)
    constraint_free (c1->orig);

  if (c2->orig != NULL)
    c1->orig = constraint_copy (c2->orig);
  else
    c1->orig = NULL;

  /*@i33 make sure that the or is freed correctly*/
  if (c1->or != NULL)
    constraint_free (c1->or);

  if (c2->or != NULL)
    c1->or = constraint_copy (c2->or);
  else
    c1->or = NULL;

  c1->fcnPre = c2->fcnPre;

  /*@-assignexpose@*/
  c1->generatingExpr = c2->generatingExpr;
  /*@=assignexpose@*/
}



static /*@notnull@*/  /*@special@*/ constraint constraint_makeNew (void)
     /*@post:isnull result->or, result->orig,  result->generatingExpr, result->fcnPre @*/ 
     /*@defines result->or, result->generatingExpr, result->orig, result->fcnPre @*/
{
  constraint ret;
  ret = dmalloc (sizeof (*ret));
  ret->lexpr = NULL;
  ret->expr = NULL;
  ret->ar = LT;
  ret->post = FALSE;
  ret->orig = NULL;
  ret->or = NULL;
  ret->generatingExpr = NULL;
  ret->fcnPre = NULL;
  return ret;
}

constraint constraint_addGeneratingExpr (/*@returned@*/ constraint c, /*@exposed@*/ exprNode e)
{
    
  if (c->generatingExpr == NULL)
    {
      c->generatingExpr = e;
      DPRINTF ((message ("setting generatingExpr for %s to %s", constraint_print (c), exprNode_unparse (e)) ));
    }
  else
    {
      DPRINTF ((message ("Not setting generatingExpr for %s to %s", constraint_print (c), exprNode_unparse (e)) ));
    }
  return c;
}

constraint constraint_origAddGeneratingExpr (/*@returned@*/ constraint c, exprNode e)
{

  if (c->orig != constraint_undefined)
    {
      c->orig = constraint_addGeneratingExpr (c->orig, e);
    }
  else
    {
      DPRINTF ((message ("constraint_origAddGeneratingExpr: Not setting generatingExpr for %s to %s", constraint_print (c), exprNode_unparse (e)) ));
    }
  return c;
}

constraint constraint_setFcnPre (/*@returned@*/ constraint c)
{

  if (c->orig != constraint_undefined)
    {
      c->orig->fcnPre = TRUE;
    }
  else
    {
      c->fcnPre = TRUE;
      DPRINTF (( message ("Warning Setting fcnPre directly")));
    }
  return c;
}




fileloc constraint_getFileloc (constraint c)
{
  if (exprNode_isDefined (c->generatingExpr))
    return (fileloc_copy (exprNode_getfileloc (c->generatingExpr)));
	    
  return (constraintExpr_getFileloc (c->lexpr));


}

static bool checkForMaxSet (constraint c)
{
  if (constraintExpr_hasMaxSet (c->lexpr) || constraintExpr_hasMaxSet (c->expr))
    return TRUE;

  return FALSE;
}

bool constraint_hasMaxSet (constraint c)
{
  if (checkForMaxSet (c))
    return TRUE;
  
  if (c->orig != NULL)
    {
      if (checkForMaxSet (c->orig))
	return TRUE;
    }

  return FALSE;
}

constraint constraint_makeReadSafeExprNode (exprNode po, exprNode ind)
{
  constraint ret = constraint_makeNew ();

  po = po;
  ind = ind;
  ret->lexpr = constraintExpr_makeMaxReadExpr (po);
  ret->ar    = GTE;
  ret->expr  = constraintExpr_makeValueExpr (ind);
  ret->post = FALSE;
  return ret;
}

constraint constraint_makeWriteSafeInt (   exprNode po, int ind)
{
  constraint ret = constraint_makeNew ();

 
  ret->lexpr =constraintExpr_makeMaxSetExpr (po);
  ret->ar = GTE;
  ret->expr =  constraintExpr_makeIntLiteral (ind);
  /*@i1*/return ret;
}

constraint constraint_makeSRefSetBufferSize (sRef s, long int size)
{
 constraint ret = constraint_makeNew ();
 ret->lexpr = constraintExpr_makeSRefMaxset (s);
 ret->ar = EQ;
 ret->expr =  constraintExpr_makeIntLiteral ((int)size);
 ret->post = TRUE;
 /*@i1*/return ret;
}

constraint constraint_makeSRefWriteSafeInt (sRef s, int ind)
{
  constraint ret = constraint_makeNew ();

 
  ret->lexpr = constraintExpr_makeSRefMaxset ( s);
  ret->ar = GTE;
  ret->expr =  constraintExpr_makeIntLiteral (ind);
  ret->post = TRUE;
  /*@i1*/return ret;
}

/* drl added 01/12/2000
   
  makes the constraint: Ensures index <= MaxRead (buffer) */

constraint constraint_makeEnsureLteMaxRead (exprNode index, exprNode buffer)
{
  constraint ret = constraint_makeNew ();

  ret->lexpr = constraintExpr_makeValueExpr (index);
  ret->ar = LTE;
  ret->expr = constraintExpr_makeMaxReadExpr (buffer);
  ret->post = TRUE;
  return ret;
}

constraint constraint_makeWriteSafeExprNode (exprNode po, exprNode ind)
{
  constraint ret = constraint_makeNew ();

 
  ret->lexpr =constraintExpr_makeMaxSetExpr (po);
  ret->ar = GTE;
  ret->expr =  constraintExpr_makeValueExpr (ind);
  /*@i1*/return ret;
}


constraint constraint_makeReadSafeInt ( exprNode t1, int index)
{
  constraint ret = constraint_makeNew ();

  ret->lexpr = constraintExpr_makeMaxReadExpr (t1);
  ret->ar    = GTE;
  ret->expr  = constraintExpr_makeIntLiteral (index);
  ret->post = FALSE;
  return ret;
}

constraint constraint_makeSRefReadSafeInt (sRef s, int ind)
{
  constraint ret = constraint_makeNew ();

 
  ret->lexpr = constraintExpr_makeSRefMaxRead (s);
  ret->ar = GTE;
  ret->expr =  constraintExpr_makeIntLiteral (ind);
  ret->post = TRUE;
  /*@i1*/return ret;
}

constraint constraint_makeEnsureMaxReadAtLeast (exprNode t1, exprNode t2, fileloc sequencePoint)
{
  constraint ret;
  
  ret = constraint_makeReadSafeExprNode (t1, t2);
  ret->lexpr = constraintExpr_setFileloc (ret->lexpr, sequencePoint);  
  ret->post = TRUE;  

  return ret;
}

static constraint constraint_makeEnsuresOpConstraintExpr (/*@only@*/ constraintExpr c1, /*@only@*/ constraintExpr c2, fileloc sequencePoint,  arithType  ar)
{

  constraint ret;
  
  llassert (constraintExpr_isDefined (c1) && constraintExpr_isDefined (c2));

  ret = constraint_makeNew ();
  
  ret->lexpr = c1;
  ret->ar = ar;
  ret->post = TRUE;
  ret->expr =  c2;
  ret->lexpr = constraintExpr_setFileloc (ret->lexpr, sequencePoint);
  return ret;
}

static constraint constraint_makeEnsuresOp (/*@dependent@*/ exprNode e1, /*@dependent@*/ exprNode e2, fileloc sequencePoint,  arithType  ar)
{
  constraintExpr c1, c2;
  constraint ret;
  exprNode e;

  if (! (exprNode_isDefined (e1) && exprNode_isDefined (e2)))
    {
      llcontbug ((message ("null exprNode, Exprnodes are %s and %s",
		       exprNode_unparse (e1), exprNode_unparse (e2))
		));
    }

  e  =  e1;
  c1 =  constraintExpr_makeValueExpr (e);
  
  e  =  e2;
  c2 =  constraintExpr_makeValueExpr (e);

  ret = constraint_makeEnsuresOpConstraintExpr (c1, c2, sequencePoint, ar);
  
  return ret;
}


/* make constraint ensures e1 == e2 */

constraint constraint_makeEnsureEqual (exprNode e1, exprNode e2, fileloc sequencePoint)
{
  return ( constraint_makeEnsuresOp (e1, e2, sequencePoint, EQ));
}

/*make constraint ensures e1 < e2 */
constraint constraint_makeEnsureLessThan (exprNode e1, exprNode e2, fileloc sequencePoint)
{
  constraintExpr t1, t2;

  t1 = constraintExpr_makeValueExpr (e1);
  t2 = constraintExpr_makeValueExpr (e2);

  /*change this to e1 <= (e2 -1) */

  t2 = constraintExpr_makeDecConstraintExpr (t2);
  
 return ( constraint_makeEnsuresOpConstraintExpr (t1, t2, sequencePoint, LTE));
}

constraint constraint_makeEnsureLessThanEqual (exprNode e1, exprNode e2, fileloc sequencePoint)
{
 return ( constraint_makeEnsuresOp (e1, e2, sequencePoint, LTE));
}

constraint constraint_makeEnsureGreaterThan (exprNode e1, exprNode e2, fileloc sequencePoint)
{
  constraintExpr t1, t2;

  t1 = constraintExpr_makeValueExpr (e1);
  t2 = constraintExpr_makeValueExpr (e2);


  /* change this to e1 >= (e2 + 1) */
  t2 = constraintExpr_makeIncConstraintExpr (t2);
  
  
 return ( constraint_makeEnsuresOpConstraintExpr (t1, t2, sequencePoint, GTE));
}

constraint constraint_makeEnsureGreaterThanEqual (exprNode e1, exprNode e2, fileloc sequencePoint)
{
 return ( constraint_makeEnsuresOp (e1, e2, sequencePoint, GTE));
}


exprNode exprNode_copyConstraints (/*@returned@*/ exprNode dst, exprNode src)
{
  constraintList_free (dst->ensuresConstraints);
  constraintList_free (dst->requiresConstraints);
  constraintList_free (dst->trueEnsuresConstraints);
  constraintList_free (dst->falseEnsuresConstraints);
  
  dst->ensuresConstraints = constraintList_copy (src->ensuresConstraints);
  dst->requiresConstraints = constraintList_copy (src->requiresConstraints);
  dst->trueEnsuresConstraints = constraintList_copy (src->trueEnsuresConstraints);
  dst->falseEnsuresConstraints = constraintList_copy (src->falseEnsuresConstraints);
  return dst;
}

/* Makes the constraint e = e + f */
constraint constraint_makeAddAssign (exprNode e, exprNode f, fileloc sequencePoint)
{
  constraintExpr x1, x2, y;
  constraint ret;

  ret = constraint_makeNew ();

  x1 =  constraintExpr_makeValueExpr (e);
  x2 =  constraintExpr_copy (x1);
  y  =  constraintExpr_makeValueExpr (f);

  ret->lexpr = x1;
  ret->ar = EQ;
  ret->post = TRUE;
  ret->expr = constraintExpr_makeAddExpr (x2, y);
  
  ret->lexpr = constraintExpr_setFileloc (ret->lexpr, sequencePoint);

  return ret;
}


/* Makes the constraint e = e - f */
constraint constraint_makeSubtractAssign (exprNode e, exprNode f, fileloc sequencePoint)
{
  constraintExpr x1, x2, y;
  constraint ret;

  ret = constraint_makeNew ();

  x1 =  constraintExpr_makeValueExpr (e);
  x2 =  constraintExpr_copy (x1);
  y  =  constraintExpr_makeValueExpr (f);

  ret->lexpr = x1;
  ret->ar = EQ;
  ret->post = TRUE;
  ret->expr = constraintExpr_makeSubtractExpr (x2, y);
  
  ret->lexpr = constraintExpr_setFileloc (ret->lexpr, sequencePoint);

  return ret;
}

constraint constraint_makeMaxSetSideEffectPostDecrement (exprNode e, fileloc sequencePoint)
{
  constraint ret = constraint_makeNew ();

  ret->lexpr = constraintExpr_makeValueExpr (e);
  ret->ar = EQ;
  ret->post = TRUE;
  ret->expr =  constraintExpr_makeValueExpr (e);
  ret->expr =  constraintExpr_makeDecConstraintExpr (ret->expr);
  ret->lexpr = constraintExpr_setFileloc (ret->lexpr, sequencePoint);
  return ret;
}
constraint constraint_makeMaxSetSideEffectPostIncrement (exprNode e, fileloc sequencePoint)
{
  constraint ret = constraint_makeNew ();

  ret->lexpr = constraintExpr_makeValueExpr (e);
  ret->ar = EQ;
  ret->post = TRUE;
  ret->expr =  constraintExpr_makeValueExpr (e);
  ret->expr =  constraintExpr_makeIncConstraintExpr (ret->expr);

  ret->lexpr = constraintExpr_setFileloc (ret->lexpr, sequencePoint);
  return ret;
}


void constraint_free (/*@only@*/ constraint c)
{
  llassert (constraint_isDefined (c));


  if (constraint_isDefined (c->orig))
    constraint_free (c->orig);
  if ( constraint_isDefined (c->or))
    constraint_free (c->or);

  
  constraintExpr_free (c->lexpr);
  constraintExpr_free (c->expr);

  c->orig = NULL;
  c->or = NULL;
  c->lexpr = NULL;
  c->expr  = NULL;

  free (c);
  
}

cstring arithType_print (arithType ar) /*@*/
{
  cstring st = cstring_undefined;
  switch (ar)
    {
    case LT:
      st = cstring_makeLiteral ("<");
      break;
    case	LTE:
      st = cstring_makeLiteral ("<=");
      break;
    case 	GT:
      st = cstring_makeLiteral (">");
      break;
    case 	GTE:
      st = cstring_makeLiteral (">=");
      break;
    case	EQ:
      st = cstring_makeLiteral ("==");
      break;
    case	NONNEGATIVE:
      st = cstring_makeLiteral ("NONNEGATIVE");
      break;
    case	POSITIVE:
      st = cstring_makeLiteral ("POSITIVE");
      break;
    default:
      llassert (FALSE);
      break;
    }
  return st;
}

void constraint_printErrorPostCondition (constraint c, fileloc loc)
{
  cstring string;
  fileloc errorLoc, temp;
  
  string = constraint_printDetailedPostCondition (c);

  errorLoc = loc;

  loc = NULL;

  temp = constraint_getFileloc (c);

    
  if (context_getFlag (FLG_BOUNDSCOMPACTERRORMESSAGES ) )
    {
      string = cstring_replaceChar(string, '\n', ' ');
    }
  
  if (fileloc_isDefined (temp))
    {
      errorLoc = temp;
      voptgenerror (  FLG_CHECKPOST, string, errorLoc);
      fileloc_free (temp);
    }
  else
    {
      voptgenerror (  FLG_CHECKPOST, string, errorLoc);
    }
}

 /*drl added 8-11-001*/
cstring constraint_printLocation (/*@observer@*/ /*@temp@*/ constraint c) /*@*/
{
  cstring string, ret;
  fileloc errorLoc;
  
  string = constraint_print (c);

  errorLoc = constraint_getFileloc (c);

  ret = message ("constraint: %q @ %q", string, fileloc_unparse (errorLoc));

  fileloc_free (errorLoc);
  return ret;

}



void constraint_printError (constraint c, fileloc loc)
{
  cstring string;
  fileloc errorLoc, temp;


  /*drl 11/26/2001 avoid printing tautological constraints */
  if (constraint_isAlwaysTrue (c))
    {
      return;
    }


  string = constraint_printDetailed (c);

  errorLoc = loc;

  temp = constraint_getFileloc (c);

  if (fileloc_isDefined (temp))
    {
      errorLoc = temp;
    }
  else
    {
      llassert (FALSE);
      DPRINTF (("constraint %s had undefined fileloc %s", constraint_print (c), fileloc_unparse (temp)));
      fileloc_free (temp);
      errorLoc = fileloc_copy (errorLoc);
    }

  
  if (context_getFlag (FLG_BOUNDSCOMPACTERRORMESSAGES ) )
    {
      string = cstring_replaceChar(string, '\n', ' ');
    }


  if (c->post)
    {
      voptgenerror (FLG_FUNCTIONPOST, string, errorLoc);
    }
  else
    {
      if (constraint_hasMaxSet (c))
	{
	  voptgenerror (FLG_BOUNDSWRITE, string, errorLoc);
	}
      else
	{
	  voptgenerror (FLG_BOUNDSREAD, string, errorLoc);
	}
    }

  fileloc_free(errorLoc);
}

static cstring constraint_printDeep (constraint c)
{
  cstring genExpr;
  cstring st = cstring_undefined;

  st = constraint_print(c);
  
  if (c->orig != constraint_undefined)
    {
      st = cstring_appendChar (st, '\n');
      genExpr =  exprNode_unparse (c->orig->generatingExpr);

      if (!c->post)
	{
	  if (c->orig->fcnPre)
	    {
	      st = cstring_concatFree (st, message (" derived from %s precondition: %q", 
						    genExpr, constraint_printDeep (c->orig)));
	    }
	  else
	    {
	      st = cstring_concatFree (st, message (" needed to satisfy precondition:\n%q",
						   constraint_printDeep (c->orig)));
	    }
	}
      else
	{
	  st = cstring_concatFree (st, message ("derived from: %q",
					       constraint_printDeep (c->orig)));
	}
    }

  return st;  
}


static /*@only@*/ cstring  constraint_printDetailedPostCondition (/*@observer@*/ /*@temp@*/ constraint c)
{
  cstring st = cstring_undefined;
  cstring genExpr;
  
  st = message ("Unsatisfied ensures constraint condition:\nSplint is unable to verify the constraint %q", constraint_printDeep (c));

  genExpr = exprNode_unparse (c->generatingExpr);
    
  if (context_getFlag (FLG_CONSTRAINTLOCATION))
    {
      cstring temp;

      temp = message ("\nOriginal Generating expression %q: %s\n", 
		      fileloc_unparse ( exprNode_getfileloc (c->generatingExpr)),
		      genExpr);
      st = cstring_concatFree (st, temp);

      if (constraint_hasMaxSet (c))
	{
	  temp = message ("Has MaxSet\n");
	  st = cstring_concatFree (st, temp);
	}
    }
  return st;
}

cstring  constraint_printDetailed (constraint c)
{
  cstring st = cstring_undefined;
  cstring temp = cstring_undefined;
  cstring genExpr;
  
  if (!c->post)
    {
      st = message ("Unable to resolve constraint:\n%q", constraint_printDeep (c));
    }
  else
    {
      st = message ("Block Post condition:\nThis function block has the post condition %q", constraint_printDeep (c));
    }

  if (constraint_hasMaxSet (c))
    {
      temp = cstring_makeLiteral ("Possible out-of-bounds store:\n");
    }
  else
    {
      temp = cstring_makeLiteral ("Possible out-of-bounds read:\n");
    }
  
  genExpr = exprNode_unparse (c->generatingExpr);
  
  if (context_getFlag (FLG_CONSTRAINTLOCATION))
    {
      cstring temp2;
      temp2 = message ("%s\n", genExpr);
      temp = cstring_concatFree (temp, temp2);
    }

  st  = cstring_concatFree (temp,st);
  
  return st;
}

/*@only@*/ cstring  constraint_print (constraint c) /*@*/
{
  cstring st = cstring_undefined;
  cstring type = cstring_undefined;
  llassert (c !=NULL);
  if (c->post)
    {
      if (context_getFlag (FLG_PARENCONSTRAINT))
	{
	  type = cstring_makeLiteral ("ensures: ");
	}
      else
	{
	   type = cstring_makeLiteral ("ensures");
	}
    }
  else
    {
      if (context_getFlag (FLG_PARENCONSTRAINT))
	{
	  type = cstring_makeLiteral ("requires: ");
	}
      else
	{
	  type = cstring_makeLiteral ("requires");
	}
	
    }
      if (context_getFlag (FLG_PARENCONSTRAINT))
	{
	  st = message ("%q: %q %q %q",
			type,
			constraintExpr_print (c->lexpr),
			arithType_print (c->ar),
			constraintExpr_print (c->expr)
			);
	}
      else
	{
	  st = message ("%q %q %q %q",
			type,
			constraintExpr_print (c->lexpr),
			arithType_print (c->ar),
			constraintExpr_print (c->expr)
		);
	}
  return st;
}

cstring  constraint_printOr (constraint c) /*@*/
{
  cstring ret;
  constraint temp;

  ret = cstring_undefined;
  temp = c;

  ret = cstring_concatFree (ret, constraint_print (temp));

  temp = temp->or;
  
  while ( constraint_isDefined (temp)) 
    {
      ret = cstring_concatFree (ret, cstring_makeLiteral (" OR "));
      ret = cstring_concatFree (ret, constraint_print (temp));
      temp = temp->or;
    }

  return ret;

}

/*@only@*/ constraint constraint_doSRefFixBaseParam (/*@returned@*/ /*@only@*/ constraint precondition,
						   exprNodeList arglist)
{
  precondition->lexpr = constraintExpr_doSRefFixBaseParam (precondition->lexpr,
							   arglist);
  precondition->expr = constraintExpr_doSRefFixBaseParam (precondition->expr,
							   arglist);

  return precondition;
}


constraint constraint_doFixResult (constraint postcondition, /*@dependent@*/ exprNode fcnCall)
{
  postcondition = constraint_copy (postcondition);
  postcondition->lexpr = constraintExpr_doFixResult (postcondition->lexpr, fcnCall);
  postcondition->expr = constraintExpr_doFixResult (postcondition->expr, fcnCall);

  return postcondition;
}
/*Commenting out temporally
  
/ *@only@* /constraint  constraint_doSRefFixInvarConstraint(constraint invar, sRef s, ctype ct )
{

  invar = constraint_copy (invar);
  invar->lexpr = constraintExpr_doSRefFixInvarConstraint (invar->lexpr, s, ct);
  invar->expr = constraintExpr_doSRefFixInvarConstraint (invar->expr, s, ct);

  return invar;
}
*/

/*@only@*/ constraint constraint_doSRefFixConstraintParam (constraint precondition,
						   exprNodeList arglist)
{

  precondition = constraint_copy (precondition);
  precondition->lexpr = constraintExpr_doSRefFixConstraintParam (precondition->lexpr, arglist);
  precondition->expr = constraintExpr_doSRefFixConstraintParam (precondition->expr, arglist);

  precondition->fcnPre = FALSE;
  return precondition;
}

constraint constraint_preserveOrig (/*@returned@*/ constraint c) /*@modifies c @*/
{

  DPRINTF ((message ("Doing constraint_preserverOrig for %q ", constraint_printDetailed (c))));
  
  if (c->orig == constraint_undefined)
    c->orig = constraint_copy (c);

  else if (c->orig->fcnPre)
    {
      constraint temp;
      
      temp = c->orig;
      
      /* avoid infinite loop */
      c->orig = NULL;
      c->orig = constraint_copy (c);
      if (c->orig->orig == NULL)
	{
	  c->orig->orig = temp;
	  temp = NULL;
	}
      else
	{
	  llcontbug ((message ("Expected c->orig->orig to be null")));
	  constraint_free (c->orig->orig);
	  c->orig->orig = temp;
	  temp = NULL;
	}
    }
  else
    {
      DPRINTF ((message ("Not changing constraint")));
    }
  
  DPRINTF ((message ("After Doing constraint_preserverOrig for %q ", constraint_printDetailed (c))));

  return c;
}
/*@=fcnuse*/
/*@=assignexpose*/
/*@=czechfcns@*/


constraint constraint_togglePost (/*@returned@*/ constraint c)
{
  c->post = !c->post;
  return c;
}

constraint constraint_togglePostOrig (/*@returned@*/ constraint c)
{
  if (c->orig != NULL)
    c->orig = constraint_togglePost (c->orig);
  return c;
}

bool constraint_hasOrig ( /*@observer@*/ /*@temp@*/ constraint c)
{
  if (c->orig == NULL)
    return FALSE;
  else
    return TRUE;
}


constraint constraint_undump (FILE *f)
{
  constraint c;
  bool           fcnPre;
  bool post;
  arithType       ar;
  
  constraintExpr lexpr;
  constraintExpr  expr;


  char * s;

  char *os;

  os = mstring_create (MAX_DUMP_LINE_LENGTH);

  s = fgets (os, MAX_DUMP_LINE_LENGTH, f);

  /*@i33*/ /*this should probably be wrappered...*/
  
  fcnPre = (bool) reader_getInt (&s);
  advanceField (&s);
  post = (bool) reader_getInt (&s);
  advanceField (&s);
  ar = (arithType) reader_getInt (&s);

  s = fgets (os, MAX_DUMP_LINE_LENGTH, f);

  reader_checkChar (&s, 'l');

  lexpr = constraintExpr_undump (f);

  s = fgets (os, MAX_DUMP_LINE_LENGTH, f);

  reader_checkChar (&s, 'r');
  expr = constraintExpr_undump (f);

  c = constraint_makeNew ();
  
  c->fcnPre = fcnPre; 
  c->post = post;
  c->ar = ar;

  c->lexpr = lexpr;
  c->expr =  expr;

  free (os);
  c = constraint_preserveOrig (c);
  return c;
}


void constraint_dump (/*@observer@*/ constraint c,  FILE *f)
{
  bool           fcnPre;
  bool post;
  arithType       ar;
  
  constraintExpr lexpr;
  constraintExpr  expr;


  fcnPre = c->fcnPre;
  post   = c->post;
  ar     = c->ar;
  lexpr = c->lexpr;
  expr = c->expr;
  
  fprintf (f, "%d@%d@%d\n", (int) fcnPre, (int) post, (int) ar);
  fprintf (f,"l\n");
  constraintExpr_dump (lexpr, f);
  fprintf (f,"r\n");
  constraintExpr_dump (expr, f);
}


int constraint_compare (/*@observer@*/ /*@temp@*/ const constraint * c1, /*@observer@*/ /*@temp@*/ const constraint * c2) /*@*/
{
  fileloc loc1, loc2;

  int ret;
  
  llassert (constraint_isDefined (*c1));
  llassert (constraint_isDefined (*c2));

  if (constraint_isUndefined (*c1))
    {
        if (constraint_isUndefined (*c2))
	  return 0;
	else
	  return 1;
    }

  if (constraint_isUndefined (*c2))
    {
      return -1;
    }
    
  loc1 = constraint_getFileloc (*c1);
  loc2 = constraint_getFileloc (*c2);

  ret = fileloc_compare (loc1, loc2);

  fileloc_free (loc1);
  fileloc_free (loc2);
    
  return ret;
}


bool constraint_isPost  (/*@observer@*/ /*@temp@*/ constraint c)
{
  llassert (constraint_isDefined (c));

  if (constraint_isUndefined (c))
    return FALSE;
  
  return (c->post);
}


static int constraint_getDepth (/*@observer@*/ /*@temp@*/ constraint c)
{
  int l , r;

  l = constraintExpr_getDepth (c->lexpr);
  r = constraintExpr_getDepth (c->expr);

  if (l > r)
    {
      DPRINTF (( message ("constraint depth returning %d for %s", l, constraint_print (c))));
      return l;
    }
  else
    {
      DPRINTF (( message ("constraint depth returning %d for %s", r, constraint_print (c))));
      return r;
    }
}


bool constraint_tooDeep (/*@observer@*/ /*@temp@*/ constraint c)
{
  int temp;

  temp = constraint_getDepth (c);

  if (temp >= 20)              
    {
      return TRUE;
    }

  return FALSE;
  
}
